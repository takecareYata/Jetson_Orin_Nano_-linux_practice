#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include "my_ioctl.h"

#define DEV_NAME "/dev/mydev"

/* sysfs에 PWM Duty Cycle(ns) 적용 함수 */
void set_sysfs_pwm(int duty_percent) {
	FILE *fp;
	long long period_ns = 1000000LL; // 1ms (1,000,000 ns)
	long long duty_ns = (period_ns * (long long)duty_percent) / 100LL;

	/* Jetson sysfs PWM duty_cycle 경로 */
	fp = fopen("/sys/class/pwm/pwmchip3/pwm0/duty_cycle", "w");
	if (fp == NULL) {
		perror("[APP Error] sysfs pwm open failed");
		return;
	}

	/* duty_cycle 값 정수 문자열로 저장 */
	fprintf(fp, "%lld\n", duty_ns);
	fflush(fp);
	fclose(fp);

	printf("  -> [sysfs PWM] Set Duty Cycle: %lld ns (%d%%)\n", duty_ns, duty_percent);
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
		usleep(2000); // 50ms 대기
	}

	close(fd);
	return EXIT_SUCCESS;
}