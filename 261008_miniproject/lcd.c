#include <stdio.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include "lcd.h"

#define LCD_ADDR      0x27
#define LCD_BACKLIGHT 0x08
#define En            0x04
#define Rw            0x02
#define Rs            0x01

static void lcd_write_four_bits(int i2c_fd, unsigned char val) {
    unsigned char data;
    ioctl(i2c_fd, I2C_SLAVE, LCD_ADDR);

    data = val;
    write(i2c_fd, &data, 1);
    usleep(50);

    data = val | En;
    write(i2c_fd, &data, 1);
    usleep(50);

    data = val & ~En;
    write(i2c_fd, &data, 1);
    usleep(50);
}

static void lcd_send(int i2c_fd, unsigned char val, unsigned char mode) {
    unsigned char high = mode | (val & 0xF0) | LCD_BACKLIGHT;
    unsigned char low  = mode | ((val << 4) & 0xF0) | LCD_BACKLIGHT;
    lcd_write_four_bits(i2c_fd, high);
    lcd_write_four_bits(i2c_fd, low);
}

static void lcd_command(int i2c_fd, unsigned char cmd) {
    lcd_send(i2c_fd, cmd, 0);
}

static void lcd_data(int i2c_fd, unsigned char data) {
    lcd_send(i2c_fd, data, Rs);
}

void lcd_init(int i2c_fd) {
    ioctl(i2c_fd, I2C_SLAVE, LCD_ADDR);
    usleep(50000);
    lcd_command(i2c_fd, 0x33);
    lcd_command(i2c_fd, 0x32);
    lcd_command(i2c_fd, 0x28); // 4-bit 모드, 2라인
    lcd_command(i2c_fd, 0x0C); // 디스플레이 ON, 커서 OFF
    lcd_command(i2c_fd, 0x06); // 커서 이동 방향 설정
    lcd_command(i2c_fd, 0x01); // 화면 클리어
    usleep(2000);
    printf("-> [I2C LCD] Initialized successfully\n");
}

void lcd_print(int i2c_fd, int row, int col, const char *str) {
    unsigned char addr = (row == 0) ? 0x80 : 0xC0;
    lcd_command(i2c_fd, addr + col);
    while (*str) {
        lcd_data(i2c_fd, *str++);
    }
}