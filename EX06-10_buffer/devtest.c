#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#include "my_ioctl.h"

#define MAX_NUM_OF_MINOR 4
#define DEFAULT_BUF_SIZE 4096
#define PRIVATEDATA

static unsigned int device_major = 120;
static unsigned int device_minor_start = 0;
static unsigned int device_minor_count = MAX_NUM_OF_MINOR;
static dev_t devt;
static struct cdev *my_cdev;
static unsigned int buf_size = DEFAULT_BUF_SIZE;

module_param(device_minor_count, uint, 0);
module_param(buf_size, uint, 0);

static struct _my_buf {
	char *buf;
	int wr;
	int rd;
} my_buf[MAX_NUM_OF_MINOR];

static long device_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
#ifdef PRIVATEDATA /* minor 번호 고려 없이 구현할 수 있다. */
	int ret = 0;
    struct _my_buf *b = (struct _my_buf *)filp->private_data;
    int free_size;

    if (!b || !b->buf)
        return -EFAULT;

    printk("devtest: device_ioctl (cmd = %u)\n", cmd);

    switch (cmd) {
        case MY_IOCTL_CMD_CLEAR_BUF:
            // 선형 버퍼 인덱스 초기화
            b->wr = 0;
            b->rd = 0;
            printk("devtest: buffer cleared\n");
            break;

        case MY_IOCTL_CMD_GET_FREE_BUF_SIZE:
            // 쓰기 가능한 남은 바이트 수 계산 (전체 크기 - 쓰기 위치)
            free_size = buf_size - b->wr;
            if (free_size < 0) free_size = 0;

            // 유저 공간 포인터(arg)로 결과 전달
            if (copy_to_user((int __user *)arg, &free_size, sizeof(int))) {
                printk("devtest: ioctl copy_to_user failed\n");
                return -EFAULT;
            }
            printk("devtest: free buffer size = %d\n", free_size);
            break;

        default:
            printk("devtest: unknown ioctl command\n");
            ret = -EINVAL;
            break;
    }

    return ret;
#else /*filp->private_data 사용을 안하는 버전 */
	int ret = 0, data;
	int minor = iminor(filp->f_path.dentry->d_inode);

	printk("devtest: device_ioctl (minor = %d)\n", minor);

	/* Implement code */
	switch(cmd) {
		case MY_IOCTL_CMD_CLEAR_BUF:
			printk("devtest: MY_IOCTL_CMD_CLEAR_BUF\n");
			my_buf[minor].wr = 0;
			my_buf[minor].rd = 0;
			break;
		case MY_IOCTL_CMD_GET_FREE_BUF_SIZE:
			printk("devtest: MY_IOCTL_CMD_GET_FREE_BUF_SIZE\n");
			data = buf_size - my_buf[minor].wr;
			if(copy_to_user((int *)arg, &data, sizeof(int))) 
			{
				return -EFAULT;
			}
			break;
		default:
			printk("devtest: unknown command\n");
			ret = -EINVAL;
			break;
	}

	return ret;
#endif
}

static ssize_t device_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
#ifdef PRIVATEDATA
	ssize_t rlen;
    struct _my_buf *b = (struct _my_buf *)filp->private_data;
    int avail_bytes;

    if (!b || !b->buf)
        return -EFAULT;

    // 읽을 수 있는 데이터 양 계산 (작성된 양 - 읽은 양)
    avail_bytes = b->wr - b->rd;
    if (avail_bytes <= 0) {
        printk("devtest: read buffer empty\n");
        return 0; // EOF
    }

    // 읽기 크기 제한: 요청 크기, 읽을 데이터 양, 최대 6바이트 중 최소값 선택
    rlen = count;
    if (rlen > 6)
        rlen = 6;
    if (rlen > avail_bytes)
        rlen = avail_bytes;

    // 유저 공간으로 복사
    if (copy_to_user(buf, b->buf + b->rd, rlen)) {
        printk("devtest: read copy_to_user failed\n");
        return -EFAULT;
    }

    b->rd += rlen; // 읽기 위치 업데이트
    printk("devtest: read %ld bytes (rd = %d, wr = %d)\n", rlen, b->rd, b->wr);

    return rlen;
#else
	ssize_t rlen; 
	int minor = iminor(filp->f_path.dentry->d_inode);

	printk("devtest: device_read (minor = %d)\n", minor);

	/* Implement code */
	rlen = my_buf[minor].wr - my_buf[minor].rd;
	if(rlen > count) 
	{
		rlen = count;
	}
	if(copy_to_user(buf, my_buf[minor].buf + my_buf[minor].rd, rlen)) 
	{
		return -EFAULT;
	}
	my_buf[minor].rd += rlen;
	printk("devtest: read %ld bytes\n", rlen);

	return rlen;
#endif
}

static ssize_t device_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
#ifdef PRIVATEDATA
	ssize_t wlen;
    struct _my_buf *b = (struct _my_buf *)filp->private_data;
    int free_space;

    if (!b || !b->buf)
        return -EFAULT;

    // 쓰기 가능한 공간 계산
    free_space = buf_size - b->wr;
    if (free_space <= 0) {
        printk("devtest: buffer full\n");
        return -ENOSPC; // No space left on device
    }

    // 쓰기 크기 제한: 요청 크기, 남은 공간, 최대 6바이트 중 최소값 선택
    wlen = count;
    if (wlen > 6)
        wlen = 6;
    if (wlen > free_space)
        wlen = free_space;

    // 유저 공간에서 데이터 가져오기
    if (copy_from_user(b->buf + b->wr, buf, wlen)) {
        printk("devtest: write copy_from_user failed\n");
        return -EFAULT;
    }

    b->wr += wlen; // 쓰기 위치 업데이트
    printk("devtest: wrote %ld bytes (rd = %d, wr = %d)\n", wlen, b->rd, b->wr);

    return wlen;

#else
	ssize_t wlen;
	int minor = iminor(filp->f_path.dentry->d_inode);

	printk("devtest: device_write (minor = %d)\n", minor);

	/* Implement code */
	wlen = buf_size - my_buf[minor].wr;
	if(wlen > count) 
	{
		wlen = count;
	}
	if(copy_from_user(my_buf[minor].buf + my_buf[minor].wr, buf, wlen)) 
	{
		return -EFAULT;
	}
	my_buf[minor].wr += wlen;
	printk("devtest: wrote %ld bytes\n", wlen);

	return wlen;
#endif
}

static int device_open(struct inode *inode, struct file *filp)
{
	int minor = iminor(inode);

	printk("devtest: device_open (minor = %d)\n", minor);

	filp->private_data = &my_buf[minor];

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

static int __init device_init(void)
{
	int ret, i;

	printk("devtest: device_init\n");
	printk("devtest: device_minor_count=%d, buf_size=%d\n", device_minor_count, buf_size);

	devt = MKDEV(device_major, device_minor_start);
	ret = register_chrdev_region(devt, device_minor_count, "my_buf");

	if(ret < 0) 
	{
		printk("devtest: can't get major %d\n", device_major);
		return ret;
	}

	my_cdev = cdev_alloc();
	my_cdev->ops = &my_fops;
	my_cdev->owner = THIS_MODULE;
	ret = cdev_add(my_cdev, devt, device_minor_count);
	if(ret) 
	{
		printk("devtest: can't add device %d\n", devt);
		unregister_chrdev_region(devt, device_minor_count);
		return ret;
	}

	for(i=0; i<device_minor_count; i++) 
	{
		my_buf[i].buf = kmalloc(buf_size, GFP_KERNEL);
		if(my_buf[i].buf == NULL) 
		{
			printk("devtest: can't alloc %d bytes\n", buf_size);
		}
	}

	return 0;
}

static void __exit device_exit(void)
{
	int i;

	printk("devtest: device_exit\n");

	for(i=0; i<device_minor_count; i++) {
		if(my_buf[i].buf) kfree(my_buf[i].buf);
	}
	cdev_del(my_cdev);
	unregister_chrdev_region(devt, device_minor_count);
}

module_init(device_init);
module_exit(device_exit);

MODULE_LICENSE("GPL");

