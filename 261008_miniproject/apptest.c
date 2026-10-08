#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include "my_ioctl.h"

#define DEV_NAME "/dev/mydev"
#define PWM_PERIOD_NS "1000000" // 1kHz (주기 1ms = 1,000,000 ns)

/* sysfs에 PWM 값 적용 지원 함수 */
void set_sysfs_pwm(int duty_percent) {
	int fd;
	char buf[64];
	long long period_ns = 1000000;
	long long duty_ns = (period_ns * duty_percent) / 100;

	/* pwmchip0 -> pwmchip3 경로로 수정 */
	fd = open("/sys/class/pwm/pwmchip3/pwm0/duty_cycle", O_WRONLY);
	if (fd >= 0) {
		snprintf(buf, sizeof(buf), "%lld", duty_ns);
		write(fd, buf, strlen(buf));
		close(fd);
	}
}

int main(int argc, char *argv[])
{
	int fd;
	int speed = 0;
	int count = 0;
	int last_speed = -1;

	fd = open(DEV_NAME, O_RDWR);
	if (fd == -1) {
		printf("apptest: %s open failed (%s)\n", DEV_NAME, strerror(errno));
		return EXIT_FAILURE;
	}
	printf("apptest: %s opened successfully\n", DEV_NAME);

	printf("=== Rotary Encoder Motor PWM Controller ===\n");
	printf("Rotate the encoder knob to change motor speed in real-time.\n");
	printf("Press Ctrl+C to exit.\n\n");

	while (1) {
		/* 커널 모듈로부터 현재 모터 속도 % 및 엔코더 카운트 읽기 */
		if (ioctl(fd, MY_IOCTL_CMD_GET_SPEED, &speed) >= 0) {
			if (speed != last_speed) {
				ioctl(fd, MY_IOCTL_CMD_GET_COUNT, &count);
				printf("[APP] Encoder Count: %d | Motor Speed: %d%%\n", count, speed);
				
				/* sysfs PWM Duty Cycle 갱신 */
				set_sysfs_pwm(speed);
				last_speed = speed;
			}
		}
		usleep(50000); // 50ms 대기
	}

	close(fd);
	return EXIT_SUCCESS;
}