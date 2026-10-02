#include <linux/ioctl.h>

#define MY_IOCTL_MAGIC 'k'

#define MY_IOCTL_CMD_GET_DIST    _IOR(MY_IOCTL_MAGIC, 1, int)
