#ifndef DS1307_H
#define DS1307_H

#include "driver/i2c_master.h"

#define DS1307_I2C_ADDR 0x68

typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day;
    uint8_t month;
    uint16_t year;
} rtc_time_t;

esp_err_t ds1307_get_time(i2c_master_dev_handle_t dev_handle, rtc_time_t *time);

#endif