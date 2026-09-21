#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"

static const char *TAG = "SERVO_LEDC";

#define SERVO_PIN 18
#define ADC1_CHAN ADC_CHANNEL_3

#define SERVO_MODE LEDC_LOW_SPEED_MODE
#define SERVO_CHANNEL LEDC_CHANNEL_0
#define SERVO_TIMER LEDC_TIMER_0
#define SERVO_FREQ 50
#define SERVO_RESOLUTION LEDC_TIMER_13_BIT

#define MIN_PULSE_US 1000
#define MAX_PULSE_US 2000
#define PERIOD_US 20000

#define ADC_DEADBAND_LOW 50
#define ADC_MAX_VAL_FOR_180_DEG 2730

static adc_oneshot_unit_handle_t adc1_handle;

static void setup_servo_ledc(void) {
    ledc_timer_config_t timer_conf = {
        .speed_mode = SERVO_MODE,
        .duty_resolution = SERVO_RESOLUTION,
        .timer_num = SERVO_TIMER,
        .freq_hz = SERVO_FREQ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

    ledc_channel_config_t channel_conf = {
        .gpio_num = SERVO_PIN,
        .speed_mode = SERVO_MODE,
        .channel = SERVO_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = SERVO_TIMER,
        .duty = 0,
        .hpoint = 0,
        .flags = {.output_invert = 0}
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_conf));
}

static void setup_adc(void) {
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten    = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC1_CHAN, &config));
}

static void set_servo_angle_ledc(float angle) {
    if (angle < 0.0f) angle = 0.0f;
    if (angle > 180.0f) angle = 180.0f;

    float pulse_us = MIN_PULSE_US + (angle / 180.0f) * (MAX_PULSE_US - MIN_PULSE_US);

    uint32_t duty = (uint32_t)((pulse_us / (float)PERIOD_US) * (1 << SERVO_RESOLUTION));

    ESP_ERROR_CHECK(ledc_set_duty(SERVO_MODE, SERVO_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(SERVO_MODE, SERVO_CHANNEL));
}

void app_main(void) {
    setup_servo_ledc();
    setup_adc();

    ESP_LOGI(TAG, "LEDC та ADC ініціалізовано на чистому ESP-IDF");

    int raw_adc = 0;

    while (1) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC1_CHAN, &raw_adc));

        int filtered_adc = raw_adc;
        if (filtered_adc < ADC_DEADBAND_LOW) {
            filtered_adc = 0;
        } else {
            filtered_adc -= ADC_DEADBAND_LOW;
        }

        if (filtered_adc > ADC_MAX_VAL_FOR_180_DEG) {
            filtered_adc = ADC_MAX_VAL_FOR_180_DEG;
        }

        float angle = ((float)filtered_adc / (float)ADC_MAX_VAL_FOR_180_DEG) * 180.0f;

        set_servo_angle_ledc(angle);

        ESP_LOGI(TAG, "Кут відхилення: %.1f deg | RAW ADC: %d", angle, raw_adc);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}