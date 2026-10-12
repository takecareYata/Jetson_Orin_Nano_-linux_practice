#ifndef LCD_H
#define LCD_H

void lcd_init(int i2c_fd);
void lcd_print(int i2c_fd, int row, int col, const char *str);

#endif