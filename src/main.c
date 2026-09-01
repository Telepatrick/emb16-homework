#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#define ADC_UNIT ADC_UNIT_1
#define ADC_CHAN ADC_CHANNEL_3
#define ADC_ATTEN ADC_ATTEN_DB_12
#define ADC_BITWIDTH ADC_BITWIDTH_12

#define V_REF 3300.0f
#define ADC_MAX_VAL 4095.0f

static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t cali_handle;

static void init_hw(void)
{
    adc_oneshot_unit_init_cfg_t init_cfg = { .unit_id = ADC_UNIT };
    adc_oneshot_new_unit(&init_cfg, &adc_handle);

    adc_oneshot_chan_cfg_t chan_cfg = { .bitwidth = ADC_BITWIDTH, .atten = ADC_ATTEN };
    adc_oneshot_config_channel(adc_handle, ADC_CHAN, &chan_cfg);

    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id = ADC_UNIT,
        .chan = ADC_CHAN,
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH,
    };
    adc_cali_create_scheme_curve_fitting(&cali_cfg, &cali_handle);
}

void app_main(void)
{
    init_hw();

    printf("| Raw Data | U calc (mV) | U read (mV) | Error (%%) |\n");
    printf("|  :---:   |    :---:    |    :---:    |   :---:   |\n");

    while (1) {
        int raw_val = 0;
        int calib_mv = 0;

        adc_oneshot_read(adc_handle, ADC_CHAN, &raw_val);
        adc_cali_raw_to_voltage(cali_handle, raw_val, &calib_mv);

        float u_calc = ((float)raw_val / ADC_MAX_VAL) * V_REF;
        float error = (calib_mv > 0) ? ((u_calc - calib_mv) / calib_mv) * 100.0f : 0.0f;

        printf("|   %4d   | %11.2f | %10d  | %+9.2f |\n", raw_val, u_calc, calib_mv, error);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}