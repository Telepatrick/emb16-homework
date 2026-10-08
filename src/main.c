#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "driver/i2c_master.h"

#include "ssd1315.h"
#include "ds1307.h"
#include "bme280.h"

#define I2C_MASTER_SCL_IO 5
#define I2C_MASTER_SDA_IO 4

static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t ssd1315_handle;
static i2c_master_dev_handle_t ds1307_handle;
static i2c_master_dev_handle_t bme280_handle;

static esp_err_t i2c_bus_init(void)
{
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    return i2c_new_master_bus(&bus_config, &bus_handle);
}

static esp_err_t register_i2c_devices(void)
{
    i2c_device_config_t ssd1315_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SSD1315_I2C_ADDR,
        .scl_speed_hz = 100000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &ssd1315_cfg, &ssd1315_handle));

    i2c_device_config_t ds1307_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = DS1307_I2C_ADDR,
        .scl_speed_hz = 100000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &ds1307_cfg, &ds1307_handle));

    i2c_device_config_t bme280_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = BME280_I2C_ADDR,
        .scl_speed_hz = 100000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &bme280_cfg, &bme280_handle));

    return ESP_OK;
}

void app_main(void)
{
    ESP_ERROR_CHECK(i2c_bus_init());
    ESP_ERROR_CHECK(register_i2c_devices());

    ssd1315_init(ssd1315_handle);
    ssd1315_clear(ssd1315_handle); 
    vTaskDelay(pdMS_TO_TICKS(50));

    bme280_init(bme280_handle);

    rtc_time_t rtc_time;
    float temp = 0.0f, press = 0.0f, hum = 0.0f;
    char display_buf[64];

    while (1) {
        if (ds1307_get_time(ds1307_handle, &rtc_time) == ESP_OK) {
            snprintf(display_buf, sizeof(display_buf), "TM:   %02d:%02d:%02d", 
                    rtc_time.hours, rtc_time.minutes, rtc_time.seconds);
            ssd1315_draw_string(ssd1315_handle, 0, 0, display_buf);
        }

        if (bme280_read_data(bme280_handle, &temp, &press, &hum) == ESP_OK) {
            snprintf(display_buf, sizeof(display_buf), "TMP:   %-5.1f C ", temp);
            ssd1315_draw_string(ssd1315_handle, 2, 0, display_buf);

            snprintf(display_buf, sizeof(display_buf), "PRSS: %-5.1f hPa", press);
            ssd1315_draw_string(ssd1315_handle, 4, 0, display_buf);

            snprintf(display_buf, sizeof(display_buf), "HUM:   %-5.1f ", hum);
            ssd1315_draw_string(ssd1315_handle, 6, 0, display_buf);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}