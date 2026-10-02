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
	int fd;

	fd = open(DEV_NAME, O_RDWR);
	if(fd == -1) {
		printf("apptest: %s (%d)\n", strerror(errno), __LINE__);
		return EXIT_FAILURE;
	}
	printf("apptest: %s opened\n", DEV_NAME);

	if(argc == 2) {
		if(strcmp(argv[1], "D") == 0) {
			int ret;
			int dist;
			
			printf("apptest: call ioctl() with MY_IOCTL_CMD_GET_DIST\n");
			ret = ioctl(fd, MY_IOCTL_CMD_GET_DIST, &dist);
			printf("apptest: return value is %d\n", ret);
			printf("apptest: distence is %d us (%f cm)\n", dist, dist / 58.0);
		}
	}

	close(fd);
	printf("apptest: %s closed\n", DEV_NAME);

	return EXIT_SUCCESS;
}
