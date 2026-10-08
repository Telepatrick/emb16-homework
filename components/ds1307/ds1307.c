#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ds1307.h"

static inline uint8_t bcd2dec(uint8_t val)
{
    return ((val / 16 * 10) + (val % 16));
}

esp_err_t ds1307_get_time(i2c_master_dev_handle_t dev_handle, rtc_time_t *time)
{
    uint8_t reg = 0x00;
    uint8_t data[7];

    esp_err_t err = i2c_master_transmit_receive(
        dev_handle, &reg, 1, data, 7, pdMS_TO_TICKS(100)
    );

    if (err == ESP_OK) {
        time->seconds = bcd2dec(data[0] & 0x7F);
        time->minutes = bcd2dec(data[1]);
        time->hours = bcd2dec(data[2] & 0x3F);
        time->day = bcd2dec(data[4]);
        time->month = bcd2dec(data[5]);
        time->year = 2000 + bcd2dec(data[6]);
    }
    return err;
}