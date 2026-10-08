#ifndef SSD1315_H
#define SSD1315_H

#include "driver/i2c_master.h"

#define SSD1315_I2C_ADDR 0x3C

esp_err_t ssd1315_init(i2c_master_dev_handle_t dev_handle);
void ssd1315_clear(i2c_master_dev_handle_t dev_handle);
void ssd1315_draw_string(i2c_master_dev_handle_t dev_handle, uint8_t page, uint8_t col, const char *str);

#endif