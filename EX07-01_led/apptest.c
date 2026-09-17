#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include "my_ioctl.h"

#define DEV_NAME "/dev/mydev"
#define RBUF_MAX 100

int main(int argc, char *argv[])
{
	int fd, len;
	char rbuf[RBUF_MAX];
	char *wbuf;
	int rlen, wlen;

	fd = open(DEV_NAME, O_RDWR);
	if(fd == -1) {
                printf("apptest: %s (%d)\n", strerror(errno), __LINE__);
                return EXIT_FAILURE;
	}
	printf("apptest: %s opened\n", DEV_NAME);

	if(argc == 3) {
		if(strcmp(argv[1], "R") == 0) {
			len = atoi(argv[2]);
			if(len > (RBUF_MAX-1)) {
				printf("apptest: read size is too big(can be read up to %d\n", RBUF_MAX-1);
				return EXIT_FAILURE;
			}

			rlen = read(fd, rbuf, len);
			if(rlen == -1) {
				printf("apptest: %s (%d)\n", strerror(errno), __LINE__);
				return EXIT_FAILURE;
			}
			rbuf[rlen] = 0;
			printf("apptest: %d bytes read [%s]\n", rlen, rbuf);
		} 
		else if(strcmp(argv[1], "W") == 0) {
			wbuf = argv[2];
			len = strlen(wbuf);

			wlen = write(fd, wbuf, len);
			if(wlen == -1) {
				printf("apptest: %s (%d)\n", strerror(errno), __LINE__);
				return EXIT_FAILURE;
			}
			wbuf[wlen] = 0;
			printf("apptest: %d bytes written [%s]\n", wlen, wbuf);
		}
		else if(strcmp(argv[1], "I") == 0) {
			int ret;
			int uarg = 12345678;

			if(strcmp(argv[2], "1") == 0) {
				printf("apptest: call ioctl() with MY_IOCTL_CMD_ONE\n");
				ret = ioctl(fd, MY_IOCTL_CMD_ONE);
				printf("apptest: return value is %d\n", ret);
			}
			else if(strcmp(argv[2], "2") == 0) {
				printf("apptest: call ioctl() with MY_IOCTL_CMD_TWO\n");
				ret = ioctl(fd, MY_IOCTL_CMD_TWO, &uarg);
				printf("apptest: return value is %d\n", ret);
			}
			else {
				printf("apptest: call ioctl() with unknown command\n");
				ret = ioctl(fd, MY_IOCTL_CMD_THREE);
				printf("apptest: return value is %d (errno = %d)\n", ret, errno);
			}
		}
		else if(strcmp(argv[1], "L") == 0) {
			int ret;

			if(strcmp(argv[2], "ON") == 0) {
				printf("apptest: call ioctl() with MY_IOCTL_CMD_LED_ON\n");
				ret = ioctl(fd, MY_IOCTL_CMD_LED_ON);
				printf("apptest: return value is %d\n", ret);
			}
			else if(strcmp(argv[2], "OFF") == 0) {
				printf("apptest: call ioctl() with MY_IOCTL_CMD_LED_OFF\n");
				ret = ioctl(fd, MY_IOCTL_CMD_LED_OFF);
				printf("apptest: return value is %d\n", ret);
			}
			else {
				printf("apptest: call ioctl() with unknown command\n");
			}
		}
	}

	close(fd);
	printf("apptest: %s closed\n", DEV_NAME);

	return EXIT_SUCCESS;
}
