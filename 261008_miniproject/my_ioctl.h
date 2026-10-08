#ifndef __MY_IOCTL_H__
#define __MY_IOCTL_H__

#include <linux/ioctl.h>

#define MY_IOCTL_MAGIC 'k'

/* 모터 속도(PWM Duty Cycle %) 및 엔코더 카운트 명령 정의 */
#define MY_IOCTL_CMD_SET_SPEED   _IOW(MY_IOCTL_MAGIC, 1, int)
#define MY_IOCTL_CMD_GET_SPEED   _IOR(MY_IOCTL_MAGIC, 2, int)
#define MY_IOCTL_CMD_GET_COUNT   _IOR(MY_IOCTL_MAGIC, 3, int)

#endif