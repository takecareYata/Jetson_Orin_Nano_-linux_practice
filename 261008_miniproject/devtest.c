#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/jiffies.h>

#include "my_ioctl.h"

#define DEVICE_NAME "my_pdev"

/* Jetson Orin Nano 핀 번호 정의 */
#define GPIO_S1_PIN     461  // S1 (엔코더)
#define GPIO_S2_PIN     460  // S2 (엔코더)
#define GPIO_SWITCH_PIN 398  // 물리 핀 12번 (스위치 버튼)

static unsigned int device_major = 120;
static unsigned int device_minor_start = 0;
static unsigned int device_minor_count = 1;
static dev_t devt;
static struct cdev *my_cdev;

static int irq_s1 = -1;
static int irq_sw = -1;

static int encoder_count = 0;
static int motor_speed_percent = 30; 
static int current_mode = 0;         // 0: Manual, 1: Temp, 2: Stop

static unsigned long last_jiffies = 0;
static unsigned long last_sw_jiffies = 0;
static int prev_s1 = -1;

/* 엔코더 ISR */
static irqreturn_t encoder_s1_isr(int irq, void *dev_id)
{
    int val_s1, val_s2;

    val_s1 = gpio_get_value(GPIO_S1_PIN);
    val_s2 = gpio_get_value(GPIO_S2_PIN);

    if (time_before(jiffies, last_jiffies + msecs_to_jiffies(3)))
        return IRQ_HANDLED;

    if (prev_s1 == 0 && val_s1 == 1) {
        last_jiffies = jiffies;

        if (val_s2 == 0) {
            encoder_count++;
            motor_speed_percent += 5;
            if (motor_speed_percent > 100) motor_speed_percent = 100;
        } else {
            encoder_count--;
            motor_speed_percent -= 5;
            if (motor_speed_percent < 0) motor_speed_percent = 0;
        }
    }

    prev_s1 = val_s1;
    return IRQ_HANDLED;
}

/* 스위치 버튼 ISR (하강 에지: 버튼이 눌릴 때) */
static irqreturn_t switch_isr(int irq, void *dev_id)
{
    // 디바운스 (200ms 이내 중복 입력 방지)
    if (time_before(jiffies, last_sw_jiffies + msecs_to_jiffies(200)))
        return IRQ_HANDLED;

    last_sw_jiffies = jiffies;

    // 모드 순환: 0 (Manual) -> 1 (Temp) -> 2 (Stop) -> 0
    current_mode = (current_mode + 1) % 3;
    pr_info("devtest: Mode toggled to Mode %d\n", current_mode);

    return IRQ_HANDLED;
}

static long device_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    int val;

    switch (cmd) {
    case MY_IOCTL_CMD_SET_SPEED:
        if (copy_from_user(&val, (void __user *)arg, sizeof(val)))
            return -EFAULT;
        if (val < 0) val = 0;
        if (val > 100) val = 100;
        motor_speed_percent = val;
        break;

    case MY_IOCTL_CMD_GET_SPEED:
        val = motor_speed_percent;
        if (copy_to_user((void __user *)arg, &val, sizeof(val)))
            return -EFAULT;
        break;

    case MY_IOCTL_CMD_GET_COUNT:
        val = encoder_count;
        if (copy_to_user((void __user *)arg, &val, sizeof(val)))
            return -EFAULT;
        break;

    case MY_IOCTL_CMD_GET_MODE: // 추가된 모드 확인용 ioctl (my_ioctl.h에 정의 필요)
        val = current_mode;
        if (copy_to_user((void __user *)arg, &val, sizeof(val)))
            return -EFAULT;
        break;

    default:
        ret = -EINVAL;
        break;
    }

    return ret;
}

static int device_open(struct inode *inode, struct file *filp) { return 0; }
static int device_release(struct inode *inode, struct file *filp) { return 0; }

static const struct file_operations my_fops = {
    .owner = THIS_MODULE,
    .open = device_open,
    .release = device_release,
    .unlocked_ioctl = device_ioctl,
};

static int __init my_module_init(void)
{
    int ret;

    devt = MKDEV(device_major, device_minor_start);
    ret = register_chrdev_region(devt, device_minor_count, "my_device");
    if (ret < 0) return ret;

    my_cdev = cdev_alloc();
    my_cdev->ops = &my_fops;
    my_cdev->owner = THIS_MODULE;
    ret = cdev_add(my_cdev, devt, device_minor_count);
    if (ret) goto err_unregister;

    /* S1 GPIO 신청 */
    ret = gpio_request(GPIO_S1_PIN, "encoder_s1");
    if (ret) goto err_cdev;
    gpio_direction_input(GPIO_S1_PIN);

    /* S2 GPIO 신청 */
    ret = gpio_request(GPIO_S2_PIN, "encoder_s2");
    if (ret) goto err_free_s1;
    gpio_direction_input(GPIO_S2_PIN);

    /* Switch GPIO 신청 (물리핀 12번 / GPIO 398) */
    ret = gpio_request(GPIO_SWITCH_PIN, "mode_switch");
    if (ret) goto err_free_s2;
    gpio_direction_input(GPIO_SWITCH_PIN);

    prev_s1 = gpio_get_value(GPIO_S1_PIN);

    /* IRQ 설정: 엔코더 S1 */
    irq_s1 = gpio_to_irq(GPIO_S1_PIN);
    if (irq_s1 < 0) { ret = irq_s1; goto err_free_sw; }
    ret = request_irq(irq_s1, encoder_s1_isr, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "encoder_s1_irq", NULL);
    if (ret) goto err_free_sw;

    /* IRQ 설정: 스위치 버튼 (풀업 상태이므로 Falling Edge 사용) */
    irq_sw = gpio_to_irq(GPIO_SWITCH_PIN);
    if (irq_sw < 0) { ret = irq_sw; goto err_free_irq_s1; }
    ret = request_irq(irq_sw, switch_isr, IRQF_TRIGGER_FALLING, "switch_irq", NULL);
    if (ret) goto err_free_irq_s1;

    pr_info("devtest: Driver loaded successfully with Switch IRQ (GPIO %d)\n", GPIO_SWITCH_PIN);
    return 0;

err_free_irq_s1:
    free_irq(irq_s1, NULL);
err_free_sw:
    gpio_free(GPIO_SWITCH_PIN);
err_free_s2:
    gpio_free(GPIO_S2_PIN);
err_free_s1:
    gpio_free(GPIO_S1_PIN);
err_cdev:
    cdev_del(my_cdev);
err_unregister:
    unregister_chrdev_region(devt, device_minor_count);
    return ret;
}

static void __exit my_module_exit(void)
{
    if (irq_sw >= 0) free_irq(irq_sw, NULL);
    if (irq_s1 >= 0) free_irq(irq_s1, NULL);
    gpio_free(GPIO_SWITCH_PIN);
    gpio_free(GPIO_S2_PIN);
    gpio_free(GPIO_S1_PIN);
    cdev_del(my_cdev);
    unregister_chrdev_region(devt, device_minor_count);
}

module_init(my_module_init);
module_exit(my_module_exit);
MODULE_LICENSE("GPL");