#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_log.h"

#define BUTTON_GPIO GPIO_NUM_16

#define DEBOUNCE_TIME_MS 50
#define POLLING_TIME_MS 10

/*
 * 1 - Raw interrupt, без debounce
 * 2 - Interrupt + time-based debounce
 * 3 - Interrupt + state-based debounce
 * 4 - Polling + state machine
 * 5 - Hardware RC + повторення одного з режимів
 */
#define DEBOUNCE_MODE 1

static const char *TAG = "BUTTON";

static volatile uint32_t interrupt_count = 0;
static volatile bool button_event = false;

static uint32_t counter = 0;

static void IRAM_ATTR button_isr_handler(void *arg)
{
#if DEBOUNCE_MODE == 1

    interrupt_count++;

#elif DEBOUNCE_MODE == 2 || DEBOUNCE_MODE == 3

    button_event = true;

#endif
}

static void button_init(void)
{
    gpio_config_t config = {
        .pin_bit_mask = 1ULL << BUTTON_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE
    };

    ESP_ERROR_CHECK(gpio_config(&config));

    #if DEBOUNCE_MODE != 4

        ESP_ERROR_CHECK(gpio_install_isr_service(0));

        ESP_ERROR_CHECK(
            gpio_isr_handler_add(
                BUTTON_GPIO,
                button_isr_handler,
                NULL
            )
        );

    #endif
}

static void task_raw_interrupt(void)
{
    static uint32_t last_count = 0;

    if (interrupt_count != last_count)
    {
        last_count = interrupt_count;

        ESP_LOGI(
            TAG,
            "Interrupt count: %lu",
            (unsigned long)last_count
        );
    }
}

static void task_time_debounce(void)
{
    static uint32_t last_event_time = 0;

    if (!button_event)
    {
        return;
    }

    button_event = false;

    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

    if (now - last_event_time < DEBOUNCE_TIME_MS)
    {
        return;
    }

    last_event_time = now;

    counter++;

    ESP_LOGI(
        TAG,
        "Accepted interrupt: %lu",
        (unsigned long)counter
    );
}

static void task_state_debounce(void)
{
    static uint32_t last_event_time = 0;

    if (!button_event)
    {
        return;
    }

    button_event = false;

    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

    if (gpio_get_level(BUTTON_GPIO) != 0)
    {
        return;
    }

    if (now - last_event_time < DEBOUNCE_TIME_MS)
    {
        return;
    }

    last_event_time = now;

    counter++;

    ESP_LOGI(
        TAG,
        "Button pressed: %lu",
        (unsigned long)counter
    );
}

typedef enum
{
    BUTTON_RELEASED,
    BUTTON_DEBOUNCING,
    BUTTON_PRESSED

} button_state_t;


static void task_polling_debounce(void)
{
    static button_state_t state = BUTTON_RELEASED;
    static uint32_t debounce_start = 0;

    uint32_t now =
        xTaskGetTickCount() * portTICK_PERIOD_MS;

    int level = gpio_get_level(BUTTON_GPIO);

    switch (state)
    {
        case BUTTON_RELEASED:

            if (level == 0)
            {
                debounce_start = now;

                state = BUTTON_DEBOUNCING;
            }

            break;


        case BUTTON_DEBOUNCING:

            if (level == 1)
            {
                state = BUTTON_RELEASED;
            }
            else if (
                now - debounce_start >=
                DEBOUNCE_TIME_MS
            )
            {
                counter++;

                ESP_LOGI(
                    TAG,
                    "Button pressed: %lu",
                    (unsigned long)counter
                );

                state = BUTTON_PRESSED;
            }

            break;


        case BUTTON_PRESSED:

            if (level == 1)
            {
                state = BUTTON_RELEASED;
            }

            break;
    }
}


void app_main(void)
{
    button_init();

    ESP_LOGI(TAG, "Button debounce test started");
    ESP_LOGI(TAG, "GPIO: %d", BUTTON_GPIO);
    ESP_LOGI(TAG, "Mode: %d", DEBOUNCE_MODE);

    #if DEBOUNCE_MODE == 1

        ESP_LOGI(TAG, "Mode 1: Raw interrupt");

    #elif DEBOUNCE_MODE == 2

        ESP_LOGI(TAG, "Mode 2: Time-based debounce");

    #elif DEBOUNCE_MODE == 3

        ESP_LOGI(TAG, "Mode 3: State-based debounce");

    #elif DEBOUNCE_MODE == 4

        ESP_LOGI(TAG, "Mode 4: Polling + state machine");

    #elif DEBOUNCE_MODE == 5

        ESP_LOGI(TAG, "Mode 5: Hardware RC + software debounce");

    #endif


    while (1)
    {
        #if DEBOUNCE_MODE == 1

                task_raw_interrupt();

        #elif DEBOUNCE_MODE == 2

                task_time_debounce();

        #elif DEBOUNCE_MODE == 3

                task_state_debounce();

        #elif DEBOUNCE_MODE == 4

                task_polling_debounce();

        #elif DEBOUNCE_MODE == 5

                task_raw_interrupt();

        #endif

        vTaskDelay(
            pdMS_TO_TICKS(POLLING_TIME_MS)
        );
    }
}