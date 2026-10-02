#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_timer.h"

#define LED_GPIO GPIO_NUM_4
#define BUTTON_GPIO GPIO_NUM_0
#define UART_TX_GPIO GPIO_NUM_17
#define UART_RX_GPIO GPIO_NUM_18

#define UART_PORT UART_NUM_1
#define UART_BAUDRATE 115200

#define UART_RX_BUFFER_SIZE 256

#define DEBOUNCE_TIME_MS 50

#define CMD_TOGGLE 'T'

static bool led_state = false;

static void led_toggle(void)
{
    led_state = !led_state;

    gpio_set_level(LED_GPIO, led_state ? 1 : 0);
}

static void init_hw(void)
{
    gpio_config_t led_config =
    {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    ESP_ERROR_CHECK(gpio_config(&led_config));

    gpio_set_level(LED_GPIO, 0);

    gpio_config_t button_config =
    {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    ESP_ERROR_CHECK(gpio_config(&button_config));

    const uart_config_t uart_config =
    {
        .baud_rate = UART_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT
    };

    ESP_ERROR_CHECK(
        uart_driver_install(
            UART_PORT,
            UART_RX_BUFFER_SIZE,
            0,
            0,
            NULL,
            0
        )
    );

    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_config));

    ESP_ERROR_CHECK(
        uart_set_pin(
            UART_PORT,
            UART_TX_GPIO,
            UART_RX_GPIO,
            UART_PIN_NO_CHANGE,
            UART_PIN_NO_CHANGE
        )
    );
}

static void send_toggle_command(void)
{
    const uint8_t command = CMD_TOGGLE;

    uart_write_bytes(UART_PORT, &command, 1);
}

static void button_task(void *arg)
{
    int last_button_state = 1;

    int64_t last_debounce_time = 0;

    bool button_pressed = false;


    while (1) {
        int button_state = gpio_get_level(BUTTON_GPIO);


        int64_t current_time = esp_timer_get_time() / 1000;

        if (button_state != last_button_state) {
            last_debounce_time = current_time;
        }

        if ((current_time - last_debounce_time) >= DEBOUNCE_TIME_MS) {
            if (button_state == 0 && !button_pressed) {
                button_pressed = true;
                send_toggle_command();
            }

            if (button_state == 1 && button_pressed) {
                button_pressed = false;
            }
        }

        last_button_state = button_state;

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void uart_receive_task(void *arg)
{
    uint8_t rx_data;

    while (1) {
        int length = uart_read_bytes(UART_PORT, &rx_data, 1, pdMS_TO_TICKS(100));

        if (length > 0) {
            if (rx_data == CMD_TOGGLE) {
                led_toggle();
            }
        }
    }
}

void app_main(void)
{
    init_hw();

    xTaskCreate(button_task, "button_task", 2048, NULL, 5, NULL);
    xTaskCreate(uart_receive_task, "uart_receive_task", 2048, NULL, 5, NULL);
}