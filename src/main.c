#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "SPI_MASTER";

#define PIN_NUM_MISO 13
#define PIN_NUM_MOSI 11
#define PIN_NUM_CLK  12
#define PIN_NUM_CS   10

typedef struct __attribute__((packed)) {
    float temperature;
    float pressure;
    float humidity;
} bme280_packet_t;

void app_main(void) {
    esp_err_t ret;
    spi_device_handle_t spi;

    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32,
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 1,
    };

    ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI Init error: %s", esp_err_to_name(ret));
        return;
    }

    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &spi);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI device adding error: %s", esp_err_to_name(ret));
        return;
    }

    bme280_packet_t rx_data;

    ESP_LOGI(TAG, "SPI Master initialized...");

    while (1) {
        memset(&rx_data, 0, sizeof(rx_data));

        spi_transaction_t t = {
            .length = sizeof(bme280_packet_t) * 8,
            .rx_buffer = &rx_data,
            .tx_buffer = NULL,
        };

        ret = spi_device_transmit(spi, &t);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Temp: %.2f °C | Press: %.2f hPa | Hum: %.2f %%",
                         rx_data.temperature, rx_data.pressure, rx_data.humidity);
        } else {
            ESP_LOGE(TAG, "SPI transmaission error: %s (код: 0x%x)", esp_err_to_name(ret), ret);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}