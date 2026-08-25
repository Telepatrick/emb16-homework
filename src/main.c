#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_task_wdt.h"

#define FAN_GPIO GPIO_NUM_40
#define LED_GPIO GPIO_NUM_4 

// Timer config (microsec: 1 sec = 1 000 000 microsec)
// For prod: 60 * 60 * 1000000ULL (1 hr) and 15 * 60 * 1000000ULL (15 min)
#define PERIOD_INTERVAL_US (10 * 1000000ULL) // repeat: 10 sec (test)
#define WORK_DURATION_US   (3 * 1000000ULL)  // Work duration: 3 sec (test)

static const char *TAG = "FAN_CONTROL";

static esp_timer_handle_t fan_off_timer = NULL;
static bool is_fan_running = false;

static void periodic_timer_callback(void* arg);
static void fan_off_timer_callback(void* arg);

static void set_fan_state(bool enable)
{
    is_fan_running = enable;
    gpio_set_level(FAN_GPIO, enable ? 1 : 0);
    gpio_set_level(LED_GPIO, enable ? 1 : 0);

    if (enable) {
        ESP_LOGI(TAG, "Motor enabled. Work duration %llu sec.", WORK_DURATION_US / 1000000ULL);
    } else {
        ESP_LOGI(TAG, "Motor disabled. Waiting next cycle...");
    }
}

static void periodic_timer_callback(void* arg)
{
    if (is_fan_running) {
        ESP_LOGW(TAG, "Attempt to start 2nd time.");
        return;
    }

    set_fan_state(true);

    esp_err_t err = esp_timer_start_once(fan_off_timer, WORK_DURATION_US);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error starting disabling timer: %s", esp_err_to_name(err));
    }
}

static void fan_off_timer_callback(void* arg)
{
    set_fan_state(false);
}

void app_main(void)
{
    gpio_reset_pin(FAN_GPIO);
    gpio_set_direction(FAN_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(FAN_GPIO, 0);

    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_GPIO, 0);

    const esp_timer_create_args_t fan_off_timer_args = {
        .callback = &fan_off_timer_callback,
        .name = "fan_off_timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&fan_off_timer_args, &fan_off_timer));

    esp_timer_handle_t periodic_timer;
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &periodic_timer_callback,
        .name = "periodic_fan_timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));

    set_fan_state(true);
    ESP_ERROR_CHECK(esp_timer_start_once(fan_off_timer, WORK_DURATION_US));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, PERIOD_INTERVAL_US));

    #if CONFIG_ESP_TASK_WDT_EN
        esp_task_wdt_config_t twdt_config = {
            .timeout_ms = 5000,
            .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
            .trigger_panic = true,
        };
        esp_task_wdt_reconfigure(&twdt_config);
        esp_task_wdt_add(NULL);
    #endif

    while (1) {
        #if CONFIG_ESP_TASK_WDT_EN
            esp_task_wdt_reset();
        #endif
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}