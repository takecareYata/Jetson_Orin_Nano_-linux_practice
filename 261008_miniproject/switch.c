#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "my_ioctl.h"

// 커널 드라이버를 통해 현재 모드 상태를 읽어옴 (0: Manual, 1: Temp, 2: Stop)
int get_current_mode(int fd) {
    int mode = 0;
    if (ioctl(fd, MY_IOCTL_CMD_GET_MODE, &mode) < 0) {
        return -1;
    }
    return mode;
}