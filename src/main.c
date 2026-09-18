#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

#define LED_GPIO GPIO_NUM_5
#define ADC_CHANNEL ADC_CHANNEL_3
#define ADC_ATTEN ADC_ATTEN_DB_12

#define SMA_WINDOW_SIZE 10

#define DARK_THRESHOLD 120
#define LIGHT_THRESHOLD 220

typedef struct {
    int buffer[SMA_WINDOW_SIZE];
    int index;
    int count;
    long sum;
} sma_filter_t;

void sma_init(sma_filter_t *filter)
{
    for (int i = 0; i < SMA_WINDOW_SIZE; i++) {
        filter->buffer[i] = 0;
    }

    filter->index = 0;
    filter->count = 0;
    filter->sum = 0;
}

int sma_update(sma_filter_t *filter, int new_val)
{
    filter->sum -= filter->buffer[filter->index];
    filter->buffer[filter->index] = new_val;
    filter->sum += new_val;

    filter->index = (filter->index + 1) % SMA_WINDOW_SIZE;

    if (filter->count < SMA_WINDOW_SIZE) {
        filter->count++;
    }

    return (int)(filter->sum / filter->count);
}

static void init_hw(adc_oneshot_unit_handle_t *adc_handle)
{
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_GPIO, 0);

    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, adc_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT, // 12-bit (0 - 4095)
        .atten = ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(*adc_handle, ADC_CHANNEL, &config));
}

void app_main(void)
{
    adc_oneshot_unit_handle_t adc1_handle;

    init_hw(&adc1_handle);

    sma_filter_t ldr_filter;
    sma_init(&ldr_filter);

    bool led_state = false;

    while (1) {
        int raw_adc = 0;
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL, &raw_adc));

        int filtered_adc = sma_update(&ldr_filter, raw_adc);

        if (!led_state && filtered_adc < DARK_THRESHOLD) {
            led_state = true;
            gpio_set_level(LED_GPIO, 1);
        } else if (led_state && filtered_adc > LIGHT_THRESHOLD) {
            led_state = false;
            gpio_set_level(LED_GPIO, 0);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }

    adc_oneshot_del_unit(adc1_handle);
}