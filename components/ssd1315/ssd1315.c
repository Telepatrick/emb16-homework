#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "font8x8.h"
#include "ssd1315.h"

static void ssd1315_write_cmd(i2c_master_dev_handle_t dev_handle, uint8_t cmd)
{
    uint8_t buf[2] = {0x00, cmd};
    i2c_master_transmit(dev_handle, buf, sizeof(buf), pdMS_TO_TICKS(100));
}

static void ssd1315_write_data(i2c_master_dev_handle_t dev_handle, uint8_t *data, size_t len)
{
    uint8_t buf[129];
    buf[0] = 0x40;
    memcpy(&buf[1], data, len);
    i2c_master_transmit(dev_handle, buf, len + 1, pdMS_TO_TICKS(100));
}

esp_err_t ssd1315_init(i2c_master_dev_handle_t dev_handle)
{
    vTaskDelay(pdMS_TO_TICKS(100));
    ssd1315_write_cmd(dev_handle, 0xAE);
    ssd1315_write_cmd(dev_handle, 0x20);
    ssd1315_write_cmd(dev_handle, 0x00);
    ssd1315_write_cmd(dev_handle, 0x8D);
    ssd1315_write_cmd(dev_handle, 0x14);
    ssd1315_write_cmd(dev_handle, 0xAF);
    return ESP_OK;
}

void ssd1315_clear(i2c_master_dev_handle_t dev_handle)
{
    if (!dev_handle) return;
    uint8_t empty_line[128] = {0};
    for (uint8_t page = 0; page < 8; page++) {
        ssd1315_write_cmd(dev_handle, 0xB0 + page);
        ssd1315_write_cmd(dev_handle, 0x00);
        ssd1315_write_cmd(dev_handle, 0x10);
        ssd1315_write_data(dev_handle, empty_line, 128);
    }
}
void ssd1315_draw_string(i2c_master_dev_handle_t dev_handle, uint8_t page, uint8_t col, const char *str) {
    if (!dev_handle || !str) return;

    ssd1315_write_cmd(dev_handle, 0xB0 + page);
    ssd1315_write_cmd(dev_handle, 0x00 + (col & 0x0F));
    ssd1315_write_cmd(dev_handle, 0x10 + ((col >> 4) & 0x0F));

    while (*str) {
        uint8_t c = (uint8_t)*str;
        if (c >= 32 && c <= 126) {
            ssd1315_write_data(dev_handle, (uint8_t *)font8x8_basic[c - 32], 8);
        } else {
            uint8_t empty[8] = {0};
            ssd1315_write_data(dev_handle, empty, 8);
        }
        str++;
    }
}