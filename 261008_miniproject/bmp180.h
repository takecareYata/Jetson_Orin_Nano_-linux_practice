#ifndef BMP180_H
#define BMP180_H

void bmp180_init(int i2c_fd);
double bmp180_read_temperature(int i2c_fd);

#endif