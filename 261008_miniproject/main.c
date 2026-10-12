#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include "my_ioctl.h"
#include "lcd.h"
#include "bmp180.h"
#include "motor.h"

#define DEV_NAME    "/dev/mydev" // 드라이버 등록 이름에 맞춤 (예: /dev/my_pdev 인 경우 수정 필요)
#define I2C_DEV     "/dev/i2c-7"

/* 모터 제어 모드 정의 */
typedef enum {
    MODE_ENCODER_SPEED = 0, // 0: 기존 엔코더 속도 제어
    MODE_TEMP_CONTROL,      // 1: 온도에 따른 모터 제어
    MODE_MOTOR_STOP,        // 2: 모터 정지 (0%)
    MODE_MAX
} MotorMode;

/* 온도를 모터 속도(0~100%)로 변환하는 함수 */
int temp_to_speed(double temp) {
    if (temp <= 20.0) return 0;
    if (temp >= 40.0) return 100;

    double step_size = (40.0 - 20.0) / 8.0; // 2.5도 간격
    int level = (int)((temp - 20.0) / step_size);

    if (level > 7) level = 7;
    if (level < 0) level = 0;

    int speed_steps[8] = {0, 14, 28, 42, 57, 71, 85, 100};
    return speed_steps[level];
}

int main(int argc, char *argv[])
{
    int fd, i2c_fd;
    int speed = 0, count = 0, last_speed = -1;
    char buf[32];
    int loop_counter = 0;
    double current_temp = 25.0; // 초기 안전값
    
    MotorMode current_mode = MODE_ENCODER_SPEED;
    int last_mode = -1;
    int mode_changed = 0;   

    // printf 버퍼링 즉시 해제 (화면 실시간 출력)
    setbuf(stdout, NULL);

    fd = open(DEV_NAME, O_RDWR);
    if (fd == -1) {
        printf("apptest: %s open failed (%s)\n", DEV_NAME, strerror(errno));
        return EXIT_FAILURE;
    }

    i2c_fd = open(I2C_DEV, O_RDWR);
    if (i2c_fd < 0) {
        perror("[APP Error] I2C open failed");
        close(fd);
        return EXIT_FAILURE;
    }

    lcd_init(i2c_fd);
    bmp180_init(i2c_fd);

    printf("=== Multi-Mode Motor Controller (Fast Temp Update) ===\n");
    printf("Press the physical switch to toggle modes. Press Ctrl+C to exit.\n\n");

    lcd_print(i2c_fd, 0, 0, "Mode: Encoder   ");
    lcd_print(i2c_fd, 1, 0, "Spd:  0% Cnt: 0   ");

    while (1) {
        /* 1. 커널 인터럽트를 통한 모드 확인 */
        int fetched_mode = get_current_mode(fd);
        if (fetched_mode >= 0) {
            current_mode = (MotorMode)fetched_mode;
        }

        if (current_mode != last_mode) {
            mode_changed = 1;
            printf("[APP] Mode Changed to: ");
            if (current_mode == MODE_ENCODER_SPEED) printf("Encoder Speed Control\n");
            else if (current_mode == MODE_TEMP_CONTROL) printf("Temperature Motor Control\n");
            else if (current_mode == MODE_MOTOR_STOP) printf("Motor Stop\n");

            last_mode = current_mode;
        }

        /* 2. 온도 측정 주기 개선 (기본 루프 20ms * 10 = 200ms, 즉 0.2초마다 갱신) */
        if (++loop_counter >= 10) { 
            current_temp = bmp180_read_temperature(i2c_fd);
            
            // 온도 모드일 때 실시간으로 LCD 온도 표시 갱신
            if (current_mode == MODE_TEMP_CONTROL) {
                snprintf(buf, sizeof(buf), "Temp: %.1f C    ", current_temp);
                lcd_print(i2c_fd, 1, 0, buf);
            }
            loop_counter = 0;
        }

        /* 3. 모드별 동작 수행 */
        switch (current_mode) {
            case MODE_ENCODER_SPEED:
                if (mode_changed) {
                    lcd_print(i2c_fd, 0, 0, "Mode: Encoder   ");
                    mode_changed = 0;
                    last_speed = -1; 
                }

                if (ioctl(fd, MY_IOCTL_CMD_GET_SPEED, &speed) >= 0) {
                    if (speed != last_speed) {
                        ioctl(fd, MY_IOCTL_CMD_GET_COUNT, &count);
                        printf("[Encoder Mode] Count: %d | Speed: %d%%\n", count, speed);
                        
                        set_sysfs_pwm(speed);

                        snprintf(buf, sizeof(buf), "Spd:%3d%% Cnt:%4d", speed, count);
                        lcd_print(i2c_fd, 1, 0, buf);

                        last_speed = speed;
                    }
                }
                break;

            case MODE_TEMP_CONTROL:
                if (mode_changed) {
                    lcd_print(i2c_fd, 0, 0, "Mode: Temp Ctrl ");
                    mode_changed = 0;
                }

                speed = temp_to_speed(current_temp);
                set_sysfs_pwm(speed);

                snprintf(buf, sizeof(buf), "Temp:%.1fC Spd:%d%", current_temp, speed);
                lcd_print(i2c_fd, 1, 0, buf);
                
                // 불필요한 usleep 제거하여 반응 속도 향상
                break;

            case MODE_MOTOR_STOP:
                if (mode_changed) {
                    lcd_print(i2c_fd, 0, 0, "Mode: STOP      ");
                    lcd_print(i2c_fd, 1, 0, "Motor Stopped   ");
                    set_sysfs_pwm(0); 
                    printf("[Stop Mode] Motor forced to 0%%\n");
                    mode_changed = 0;
                }
                break;

            default:
                break;
        }

        usleep(20000); // 기본 루프 대기 시간을 20ms로 조정하여 전체적인 반응성 향상
    }

    close(i2c_fd);
    close(fd);
    return EXIT_SUCCESS;
}