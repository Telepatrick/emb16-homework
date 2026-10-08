#ifndef BME280_H
#define BME280_H

#include "driver/i2c_master.h"
#include "esp_err.h"

#define BME280_I2C_ADDR 0x76

esp_err_t bme280_init(i2c_master_dev_handle_t dev_handle);
esp_err_t bme280_read_data(i2c_master_dev_handle_t dev_handle, float *temperature, float *pressure, float *humidity);

#endif