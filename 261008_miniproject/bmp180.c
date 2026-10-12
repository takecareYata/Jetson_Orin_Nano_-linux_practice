#include <stdio.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include "bmp180.h"

#define BMP180_ADDR 0x77

static short ac1, ac2, ac3;
static unsigned short ac4, ac5, ac6;
static short b1, b2;
static short mb, mc, md;

static short bmp180_read_word(int i2c_fd, unsigned char reg) {
    unsigned char buf[2];
    ioctl(i2c_fd, I2C_SLAVE, BMP180_ADDR);
    write(i2c_fd, &reg, 1);
    read(i2c_fd, buf, 2);
    return (short)((buf[0] << 8) + buf[1]);
}

void bmp180_init(int i2c_fd) {
    ioctl(i2c_fd, I2C_SLAVE, BMP180_ADDR);
    ac1 = bmp180_read_word(i2c_fd, 0xAA);
    ac2 = bmp180_read_word(i2c_fd, 0xAC);
    ac3 = bmp180_read_word(i2c_fd, 0xAE);
    ac4 = (unsigned short)bmp180_read_word(i2c_fd, 0xB0);
    ac5 = (unsigned short)bmp180_read_word(i2c_fd, 0xB2);
    ac6 = (unsigned short)bmp180_read_word(i2c_fd, 0xB4);
    b1  = bmp180_read_word(i2c_fd, 0xB6);
    b2  = bmp180_read_word(i2c_fd, 0xB8);
    mb  = bmp180_read_word(i2c_fd, 0xBA);
    mc  = bmp180_read_word(i2c_fd, 0xBC);
    md  = bmp180_read_word(i2c_fd, 0xBE);
    printf("-> [BMP180] Initialized and calibration data loaded\n");
}

double bmp180_read_temperature(int i2c_fd) {
    unsigned char reg[2];
    long ut;
    long x1, x2, b5;
    double temp;

    ioctl(i2c_fd, I2C_SLAVE, BMP180_ADDR);

    reg[0] = 0xF4;
    reg[1] = 0x2E;
    write(i2c_fd, reg, 2);
    usleep(5000); // 4.5ms 대기

    unsigned char dest_reg = 0xF6;
    write(i2c_fd, &dest_reg, 1);
    unsigned char read_buf[2];
    read(i2c_fd, read_buf, 2);
    ut = (long)((read_buf[0] << 8) + read_buf[1]);

    x1 = ((ut - (long)ac6) * (long)ac5) >> 15;
    x2 = ((long)mc << 11) / (x1 + (long)md);
    b5 = x1 + x2;
    temp = ((b5 + 8) >> 4) / 10.0;

    return temp;
}