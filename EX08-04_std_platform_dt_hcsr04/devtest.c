#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <asm/io.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>

#include "my_ioctl.h"

#define MAX_BUF 26

static unsigned int device_major = 120;
static unsigned int device_minor_start = 0;
static unsigned int device_minor_count = 1;
static dev_t devt;
static struct cdev *my_cdev;

#define GPIO_PHY_BASE           0x02212880
#define GPIO_PHY_SIZE           0x20
#define GPIO_ENABLE_CONFIG      0x00
#define GPIO_OUTPUT_CONTROL     0x0c
#define GPIO_OUTPUT_VALUE       0x10

#define CONF_REQUEST_MEM_REGION_EN 0

static volatile unsigned long gpio_base;
#if CONF_REQUEST_MEM_REGION_EN
static struct resource *gpio_mem;
#endif

int irq_hcsr04;
static int irq_enabled = 0;

#include <linux/sched.h>
#include <linux/wait.h>
static DECLARE_WAIT_QUEUE_HEAD(my_waitqueue);
static int my_flag;

#include <linux/platform_device.h>
#define DEVICE_NAME "my_pdev"

static int phy_base;
static int phy_size;
static int irq;
static int isr_count;

static void hcsr04_init(void)
{
	/* Implement code */
	printk("devtest: HC-SR04 init start\n");
	iowrite32((ioread32((void *)(gpio_base+GPIO_OUTPUT_VALUE)) & ~(0x1<<0)), (void *)(gpio_base+GPIO_OUTPUT_VALUE));
	iowrite32((ioread32((void *)(gpio_base+GPIO_OUTPUT_CONTROL)) & ~(0x1<<0)), (void *)(gpio_base+GPIO_OUTPUT_CONTROL));
	iowrite32((ioread32((void *)(gpio_base+GPIO_ENABLE_CONFIG)) & ~(0x3<<0)) | (0x3<<0), (void *)(gpio_base+GPIO_ENABLE_CONFIG));
	printk("devtest: HC-SR04 init end\n");
}

static void triger_high(void)
{
	/* Implement code */
	iowrite32((ioread32((void *)(gpio_base+GPIO_OUTPUT_VALUE)) | (0x1<<0)), (void *)(gpio_base+GPIO_OUTPUT_VALUE));
}

static void triger_low(void)
{
	/* Implement code */
	iowrite32((ioread32((void *)(gpio_base+GPIO_OUTPUT_VALUE)) & ~(0x1<<0)), (void *)(gpio_base+GPIO_OUTPUT_VALUE));
}

static long device_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	unsigned long dist_us = 0;

	printk("devtest: device_ioctl (minor = %d)\n", iminor(filp->f_path.dentry->d_inode));
	switch(cmd) {
		case MY_IOCTL_CMD_GET_DIST:
			printk("devtest: MY_IOCTL_CMD_GET_DIST\n");

			/* Implement code */

			// my_flag를 0으로 초기화 및 기타 변수 초기화
			my_flag = 0;
			isr_count = 0;

			// 10us 동안 트리거 신호를 high로 유지한 후 low로 전환
			triger_high();
			udelay(10);
			triger_low();

			// Interruptible sleep 상태에서 my_flag가 0이 아닌 경우까지 대기
			// 인터럽트 핸들러에서 my_flag를 1로 설정하고 wake_up_interruptible(&my_waitqueue);를 호출하면 깨어남
			printk("devtest: waiting key event\n");
			if(wait_event_interruptible(my_waitqueue, my_flag != 0)) {
				printk("devtest: interrupted\n");
				return -ERESTARTSYS;
			}
			printk("devtest: awoken!\n");

			// 측정된 값을 사용자 공간으로 복사
			// hcsr04_isr에서 구한 계산 결과(my_flag 값 등)를 바탕으로 거리 복사
			dist_us = (unsigned long)my_flag;
			if (copy_to_user((void __user *)arg, &dist_us, sizeof(dist_us))) {
				return -EFAULT;
			}

			break;
		default:
			printk("devtest: unknown command\n");
			ret = -EINVAL;
			break;
	}

	return ret;
}

static ssize_t device_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	printk("devtest: device_read (minor = %d)\n", iminor(filp->f_path.dentry->d_inode));
	return 0;
}

static ssize_t device_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	printk("devtest: device_write (minor = %d)\n", iminor(filp->f_path.dentry->d_inode));
	return 0;
}

static int device_open(struct inode *inode, struct file *filp)
{
	printk("devtest: device_open (minor = %d)\n", iminor(inode));
	return 0;
}

static int device_release(struct inode *inode, struct file *filp)
{
	printk("devtest: device_release\n");
	return 0;
}


static const struct file_operations my_fops = {
	.owner = THIS_MODULE,
	.open = device_open,
	.release = device_release,
	.read = device_read,
	.write = device_write,
	.unlocked_ioctl = device_ioctl,
};

irqreturn_t hcsr04_isr(int irq, void *dev_id)
{
	/* Implement code */

	// 필요한 변수 선언 (구현 필요)
	static ktime_t t_start, t_end;
	unsigned long long echo_time_ns = 0;
	unsigned long long echo_time_us = 0;

	// 인터럽트 발생 시 isr_count를 1 증가시킴
	printk("devtest: %s(): isr_count = %d\n", __FUNCTION__, ++isr_count);

	// isr_count를 보고 rising edge인지 falling edge인지 판단하고, 측정 시작/종료 및 거리 계산 수행 (구현 필요)
	// 참고: ktime_get() 함수를 사용하여 현재 시간을 가져올 수 있음
	// 참고: my_flag를 1로 설정하고 wake_up_interruptible(&my_waitqueue);를 호출하여 대기 중인 프로세스를 깨움
	if (isr_count == 1) {
		// Rising edge: Echo 핀 High 시작 시점 기록
		t_start = ktime_get();
	} 
	else if (isr_count == 2) {
		// Falling edge: Echo 핀 Low 종료 시점 기록
		t_end = ktime_get();

		// 시간 차이 계산 (ns -> us)
		echo_time_ns = t_end - t_start;
		echo_time_us = echo_time_ns/1000;
		my_flag = (int)echo_time_us;

		// 대기 중인 프로세스를 깨움
		wake_up_interruptible(&my_waitqueue);
	}

	return IRQ_HANDLED;
}

static const struct of_device_id my_pdev_of_match[] = {
	{ .compatible = "my-pdev", },
	{ },
};
MODULE_DEVICE_TABLE(of, my_pdev_of_match);

static int driver_probe(struct platform_device *pdev)
{
	int ret;
	struct resource *res;

	printk("devtest: device_init\n");

	devt = MKDEV(device_major, device_minor_start);
	ret = register_chrdev_region(devt, device_minor_count, "my_device");
	if(ret < 0) {
		printk("devtest: can't get major %d\n", device_major);
		goto err0;
	}

	my_cdev = cdev_alloc();
	my_cdev->ops = &my_fops;
	my_cdev->owner = THIS_MODULE;
	ret = cdev_add(my_cdev, devt, device_minor_count);
	if(ret) {
		printk("devtest: can't add device %d\n", devt);
		goto err1;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		printk("devtest: can't allocate resource\n");
		ret = -ENODEV;
		goto err2;
	}
	phy_base = res->start;
	phy_size = res->end - res->start;

	irq = platform_get_irq(pdev, 0);
	if (irq <= 0) {
		printk("devtest: can't find IRQ\n");
		ret = -ENODEV;
		goto err2;
	}

#if CONF_REQUEST_MEM_REGION_EN
	gpio_mem = request_mem_region(phy_base, phy_size, "gpio");
	if (gpio_mem == NULL) {
		printk("devtest: failed to get memory region\n");
		ret = -EIO;
		goto err2;
	}
#endif

	gpio_base = (unsigned long)ioremap(phy_base, phy_size);
	if (gpio_base == 0) {
		printk("devtest: ioremap error\n");
		ret = -EIO;
		goto err3;
	}

	hcsr04_init();

	irq_hcsr04 = irq;
	if(request_irq(irq_hcsr04, hcsr04_isr, 0, "hcsr04_int", NULL)) {
		printk("devtest: IRQ %d is not free\n", irq_hcsr04);
		ret = -EIO;
		goto err4;
	}
	printk("devtest: IRQ %d is enabled\n", irq_hcsr04);
	irq_enabled = 1;

	return 0;

err4:
	iounmap((void *)gpio_base);
err3:
#if CONF_REQUEST_MEM_REGION_EN
	release_mem_region(phy_base, phy_size);
#endif
err2:
	cdev_del(my_cdev);
err1:
	unregister_chrdev_region(devt, device_minor_count);
err0:
	return ret;
}

static int driver_remove(struct platform_device *pdev)
{
	printk("devtest: device_exit\n");

	free_irq(irq_hcsr04, NULL);
	irq_enabled = 0;
	iounmap((void *)gpio_base);
#if CONF_REQUEST_MEM_REGION_EN
	release_mem_region(phy_base, phy_size);
#endif
	cdev_del(my_cdev);
	unregister_chrdev_region(devt, device_minor_count);

	return 0;
}

static int driver_suspend(struct platform_device *pdev, pm_message_t pm)
{
	return 0;
}

static int driver_resume(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver my_platform_driver = {
	.probe          = driver_probe,
	.remove         = driver_remove,
	.suspend        = driver_suspend,
	.resume         = driver_resume,
	.driver         = {
		.name   = DEVICE_NAME,
		.owner  = THIS_MODULE,
		.of_match_table = my_pdev_of_match,
	},
};

static int __init platform_device_init(void)
{
	int ret;

	printk("devtest: platform_device_init\n");

	ret = platform_driver_register(&my_platform_driver);
	if(ret) {
		printk("platform_driver_register failed (ret=%d)\n", ret);
	}

	return ret;
}

static void __exit platform_device_exit(void)
{
	printk("devtest: platform_device_exit\n");

	platform_driver_unregister(&my_platform_driver);
}

module_init(platform_device_init);
module_exit(platform_device_exit);

MODULE_LICENSE("GPL");
