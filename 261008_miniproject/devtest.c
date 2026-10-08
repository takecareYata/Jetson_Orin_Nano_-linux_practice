#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/jiffies.h> // 디바운스 타임스탬프용

#include "my_ioctl.h"

#define DEVICE_NAME "my_pdev"

/* Jetson Orin Nano J12 Header GPIO 번호 */
#define GPIO_S1_PIN  461  // 물리 핀 11번 (S1 / CLK)
#define GPIO_S2_PIN  329  // 물리 핀 31번 (S2 / DT)

static unsigned int device_major = 120;
static unsigned int device_minor_start = 0;
static unsigned int device_minor_count = 1;
static dev_t devt;
static struct cdev *my_cdev;

static int irq_s1 = -1;
static int encoder_count = 0;
static int motor_speed_percent = 30; // 기본 Duty 30%

/* 디바운싱용 타임스탬프 변수 */
static unsigned long last_jiffies = 0;

/* 엔코더 ISR */
static irqreturn_t encoder_s1_isr(int irq, void *dev_id)
{
	int val_s1, val_s2;

	/* 디바운스 처리: 5ms 이내 재호출 무시 (스위치 노이즈 방지) */
	if (time_before(jiffies, last_jiffies + msecs_to_jiffies(5)))
		return IRQ_HANDLED;

	last_jiffies = jiffies;

	val_s1 = gpio_get_value(GPIO_S1_PIN);
	val_s2 = gpio_get_value(GPIO_S2_PIN);

	/* S1과 S2의 상태 차이로 방향 판별 */
	if (val_s1 != val_s2) {
		/* 시계 방향 (CW) */
		encoder_count++;
		motor_speed_percent += 5;
		if (motor_speed_percent > 100) motor_speed_percent = 100;
	} else {
		/* 반시계 방향 (CCW) */
		encoder_count--;
		motor_speed_percent -= 5;
		if (motor_speed_percent < 0) motor_speed_percent = 0;
	}

	pr_info("devtest: Encoder Count = %d, Motor Speed = %d%%\n", 
		encoder_count, motor_speed_percent);

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
		pr_info("devtest: Set Motor Speed to %d%%\n", motor_speed_percent);
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

	default:
		pr_warn("devtest: unknown ioctl command\n");
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int device_open(struct inode *inode, struct file *filp)
{
	pr_info("devtest: device_open\n");
	return 0;
}

static int device_release(struct inode *inode, struct file *filp)
{
	pr_info("devtest: device_release\n");
	return 0;
}

static const struct file_operations my_fops = {
	.owner = THIS_MODULE,
	.open = device_open,
	.release = device_release,
	.unlocked_ioctl = device_ioctl,
};

static int __init my_module_init(void)
{
	int ret;

	pr_info("devtest: module_init start\n");

	devt = MKDEV(device_major, device_minor_start);
	ret = register_chrdev_region(devt, device_minor_count, "my_device");
	if (ret < 0) {
		pr_err("devtest: can't get major %d\n", device_major);
		return ret;
	}

	my_cdev = cdev_alloc();
	my_cdev->ops = &my_fops;
	my_cdev->owner = THIS_MODULE;
	ret = cdev_add(my_cdev, devt, device_minor_count);
	if (ret) {
		pr_err("devtest: cdev_add failed\n");
		goto err_unregister;
	}

	/* 1. GPIO Request & Direction Setup */
	ret = gpio_request(GPIO_S1_PIN, "encoder_s1");
	if (ret) {
		pr_err("devtest: failed to request GPIO %d (Error: %d)\n", GPIO_S1_PIN, ret);
		goto err_cdev;
	}
	gpio_direction_input(GPIO_S1_PIN);

	ret = gpio_request(GPIO_S2_PIN, "encoder_s2");
	if (ret) {
		pr_err("devtest: failed to request GPIO %d (Error: %d)\n", GPIO_S2_PIN, ret);
		goto err_free_s1;
	}
	gpio_direction_input(GPIO_S2_PIN);

	/* 2. S1 핀의 IRQ 할당 및 인터럽트 등록 (Rising / Falling 둘 다 감지하도록 수정) */
	irq_s1 = gpio_to_irq(GPIO_S1_PIN);
	if (irq_s1 < 0) {
		pr_err("devtest: failed to map GPIO %d to IRQ\n", GPIO_S1_PIN);
		ret = irq_s1;
		goto err_free_s2;
	}

	ret = request_irq(irq_s1, encoder_s1_isr, 
			  IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, 
			  "encoder_s1_irq", NULL);
	if (ret) {
		pr_err("devtest: failed to request IRQ %d\n", irq_s1);
		goto err_free_s2;
	}

	pr_info("devtest: Rotary Encoder Driver loaded (S1: IRQ %d, S2: GPIO %d)\n", 
		irq_s1, GPIO_S2_PIN);
	return 0;

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
	pr_info("devtest: module_exit\n");

	if (irq_s1 >= 0)
		free_irq(irq_s1, NULL);

	gpio_free(GPIO_S2_PIN);
	gpio_free(GPIO_S1_PIN);

	cdev_del(my_cdev);
	unregister_chrdev_region(devt, device_minor_count);
}

module_init(my_module_init);
module_exit(my_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jetson Developer");
MODULE_DESCRIPTION("Rotary Encoder Driver using Sysfs GPIO");