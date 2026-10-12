#include <stdio.h>
#include "motor.h"

void set_sysfs_pwm(int duty_percent) {
    FILE *fp;
    long long period_ns = 1000000LL; // 1ms (1,000,000 ns)
    long long duty_ns = (period_ns * (long long)duty_percent) / 100LL;

    fp = fopen("/sys/class/pwm/pwmchip3/pwm0/duty_cycle", "w");
    if (fp == NULL) {
        perror("[APP Error] sysfs pwm open failed");
        return;
    }

    fprintf(fp, "%lld\n", duty_ns);
    fflush(fp);
    fclose(fp);
}