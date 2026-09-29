#include <stdio.h>
#include <stdbool.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "driver/ledc.h"
#include "driver/pulse_cnt.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#define PIN_ENC_A GPIO_NUM_9
#define PIN_ENC_B GPIO_NUM_10
#define PIN_ENC_SW GPIO_NUM_11
#define PIN_SERVO_PWM GPIO_NUM_18
#define PIN_BUZZER_PWM GPIO_NUM_5

#define PWM_TIMER_SERVO LEDC_TIMER_0
#define PWM_CHAN_SERVO LEDC_CHANNEL_0
#define PWM_TIMER_BUZZER LEDC_TIMER_1
#define PWM_CHAN_BUZZER LEDC_CHANNEL_1

#define ANGLE_MIN_VAL 0
#define ANGLE_MAX_VAL 180
#define ANGLE_MID_VAL 90

#define SERVO_PULSE_MIN_US 500
#define SERVO_PULSE_MAX_US 2500
#define SERVO_PWM_PERIOD_US 20000

#define BEEP_FREQ_HZ 2500
#define BEEP_TIME_MS 100
#define HOLD_TIME_MS 800
#define DEBOUNCE_MS 40
#define DEFAULT_STEP 10.0f

typedef struct {
    float current_angle;
    bool fine_step;
    pcnt_unit_handle_t pcnt_unit;
    esp_timer_handle_t timer_beep;
} app_ctx_t;

static app_ctx_t g_ctx = {
    .current_angle = ANGLE_MID_VAL,
    .fine_step = false,
    .pcnt_unit = NULL,
    .timer_beep = NULL
};

static void servo_write_angle(float deg)
{
    if (deg < ANGLE_MIN_VAL) deg = ANGLE_MIN_VAL;
    if (deg > ANGLE_MAX_VAL) deg = ANGLE_MAX_VAL;
    float pulse = SERVO_PULSE_MIN_US + (deg / (float)ANGLE_MAX_VAL) * (SERVO_PULSE_MAX_US - SERVO_PULSE_MIN_US);
    uint32_t duty = (uint32_t)((pulse / SERVO_PWM_PERIOD_US) * 4095.0f);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, PWM_CHAN_SERVO, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, PWM_CHAN_SERVO);
}

static void beeper_stop_callback(void *arg)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, PWM_CHAN_BUZZER, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, PWM_CHAN_BUZZER);
}

static void sound_alert(void)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, PWM_CHAN_BUZZER, 512);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, PWM_CHAN_BUZZER);
    esp_timer_stop(g_ctx.timer_beep);
    esp_timer_start_once(g_ctx.timer_beep, BEEP_TIME_MS * 1000);
}

static void init_hw(void)
{
    pcnt_unit_config_t u_cfg = {
        .high_limit = 30000,
        .low_limit = -30000,
        .flags.accum_count = true
    };
    pcnt_new_unit(&u_cfg, &g_ctx.pcnt_unit);

    pcnt_channel_handle_t ch_a = NULL;
    pcnt_channel_handle_t ch_b = NULL;

    pcnt_chan_config_t c_cfg_a = {
        .edge_gpio_num = PIN_ENC_A,
        .level_gpio_num = PIN_ENC_B,
    };
    pcnt_new_channel(g_ctx.pcnt_unit, &c_cfg_a, &ch_a);

    pcnt_chan_config_t c_cfg_b = {
        .edge_gpio_num = PIN_ENC_B,
        .level_gpio_num = PIN_ENC_A,
    };
    pcnt_new_channel(g_ctx.pcnt_unit, &c_cfg_b, &ch_b);

    pcnt_channel_set_edge_action(ch_a, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE);
    pcnt_channel_set_level_action(ch_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);

    pcnt_channel_set_edge_action(ch_b, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE);
    pcnt_channel_set_level_action(ch_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);

    pcnt_glitch_filter_config_t f_cfg = {
        .max_glitch_ns = 2000,
    };
    pcnt_unit_set_glitch_filter(g_ctx.pcnt_unit, &f_cfg);
    pcnt_unit_enable(g_ctx.pcnt_unit);
    pcnt_unit_clear_count(g_ctx.pcnt_unit);
    pcnt_unit_start(g_ctx.pcnt_unit);

    ledc_timer_config_t t_servo = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_12_BIT,
        .timer_num = PWM_TIMER_SERVO,
        .freq_hz = 50,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&t_servo);

    ledc_channel_config_t c_servo = {
        .gpio_num = PIN_SERVO_PWM,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = PWM_CHAN_SERVO,
        .timer_sel = PWM_TIMER_SERVO,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&c_servo);

    ledc_timer_config_t t_buzzer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = PWM_TIMER_BUZZER,
        .freq_hz = BEEP_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&t_buzzer);

    ledc_channel_config_t c_buzzer = {
        .gpio_num = PIN_BUZZER_PWM,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = PWM_CHAN_BUZZER,
        .timer_sel = PWM_TIMER_BUZZER,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&c_buzzer);

    esp_timer_create_args_t tmr_args = {
        .callback = beeper_stop_callback,
        .name = "beeper"
    };
    esp_timer_create(&tmr_args, &g_ctx.timer_beep);
}

static void encoder_btn_worker(void *param)
{
    gpio_config_t g_cfg = {
        .pin_bit_mask = (1ULL << PIN_ENC_SW),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&g_cfg);

    TickType_t t_down = 0;
    bool active = false;

    while (1) {
        int val = gpio_get_level(PIN_ENC_SW);
        
        if (val == 0 && !active) {
            vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));

            if (gpio_get_level(PIN_ENC_SW) == 0) {
                active = true;
                t_down = xTaskGetTickCount();
            }
        } else if (val == 1 && active) {
            vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
            if (gpio_get_level(PIN_ENC_SW) == 1) {
                TickType_t elapsed = xTaskGetTickCount() - t_down;

                if (pdTICKS_TO_MS(elapsed) >= HOLD_TIME_MS) {
                    g_ctx.current_angle = ANGLE_MID_VAL;
                    servo_write_angle(g_ctx.current_angle);
                } else {
                    g_ctx.fine_step = !g_ctx.fine_step;
                }

                active = false;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(15));
    }
}

void app_main(void)
{
    init_hw();

    servo_write_angle(g_ctx.current_angle);

    xTaskCreate(encoder_btn_worker, "btn_wrk", 2048, NULL, 5, NULL);

    int last_cnt = 0;
    int raw_cnt = 0;
    int pulse_accumulator = 0;

    while (1) {
        pcnt_unit_get_count(g_ctx.pcnt_unit, &raw_cnt);
        int diff = raw_cnt - last_cnt;

        if (diff != 0) {
            last_cnt = raw_cnt;
            pulse_accumulator += diff;

            int steps = pulse_accumulator / 4;
            if (steps != 0) {
                float step_size = g_ctx.fine_step ? (DEFAULT_STEP / 2.0f) : DEFAULT_STEP;
                float target = g_ctx.current_angle + (steps * step_size);

                if (target < ANGLE_MIN_VAL || target > ANGLE_MAX_VAL) {
                    sound_alert();
                }

                if (target < ANGLE_MIN_VAL) target = ANGLE_MIN_VAL;
                if (target > ANGLE_MAX_VAL) target = ANGLE_MAX_VAL;

                g_ctx.current_angle = target;
                servo_write_angle(g_ctx.current_angle);

                pulse_accumulator -= (steps * 4);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(30));
    }
}