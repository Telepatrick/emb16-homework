#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "bme280.h"

static const char *TAG = "BME280";

#define BME280_REG_CALIB00      0x88
#define BME280_REG_CALIB26      0xE1
#define BME280_REG_DIG_H1       0xA1
#define BME280_REG_CHIP_ID      0xD0
#define BME280_REG_CTRL_HUM     0xF2
#define BME280_REG_CTRL_MEAS    0xF4
#define BME280_REG_CONFIG       0xF5
#define BME280_REG_PRESS_MSB    0xF7

#define BME280_CHIP_ID_VAL      0x60

typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;

    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;

    uint8_t  dig_H1;
    int16_t  dig_H2;
    uint8_t  dig_H3;
    int16_t  dig_H4;
    int16_t  dig_H5;
    int8_t   dig_H6;
} bme280_calib_t;

static bme280_calib_t s_calib;
static int32_t s_t_fine;

static esp_err_t bme280_read_reg(i2c_master_dev_handle_t dev_handle, uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(dev_handle, &reg, 1, data, len, pdMS_TO_TICKS(100));
}

static esp_err_t bme280_write_reg(i2c_master_dev_handle_t dev_handle, uint8_t reg, uint8_t data)
{
    uint8_t buf[2] = {reg, data};
    return i2c_master_transmit(dev_handle, buf, sizeof(buf), pdMS_TO_TICKS(100));
}

static esp_err_t bme280_read_calibration(i2c_master_dev_handle_t dev_handle)
{
    uint8_t buf[24];
    
    esp_err_t err = bme280_read_reg(dev_handle, BME280_REG_CALIB00, buf, 24);
    if (err != ESP_OK) return err;

    s_calib.dig_T1 = (uint16_t)((buf[1] << 8) | buf[0]);
    s_calib.dig_T2 = (int16_t)((buf[3] << 8) | buf[2]);
    s_calib.dig_T3 = (int16_t)((buf[5] << 8) | buf[4]);

    s_calib.dig_P1 = (uint16_t)((buf[7] << 8) | buf[6]);
    s_calib.dig_P2 = (int16_t)((buf[9] << 8) | buf[8]);
    s_calib.dig_P3 = (int16_t)((buf[11] << 8) | buf[10]);
    s_calib.dig_P4 = (int16_t)((buf[13] << 8) | buf[12]);
    s_calib.dig_P5 = (int16_t)((buf[15] << 8) | buf[14]);
    s_calib.dig_P6 = (int16_t)((buf[17] << 8) | buf[16]);
    s_calib.dig_P7 = (int16_t)((buf[19] << 8) | buf[18]);
    s_calib.dig_P8 = (int16_t)((buf[21] << 8) | buf[20]);
    s_calib.dig_P9 = (int16_t)((buf[23] << 8) | buf[22]);

    err = bme280_read_reg(dev_handle, BME280_REG_DIG_H1, &s_calib.dig_H1, 1);
    if (err != ESP_OK) return err;

    uint8_t h_buf[7];
    err = bme280_read_reg(dev_handle, BME280_REG_CALIB26, h_buf, 7);
    if (err != ESP_OK) return err;

    s_calib.dig_H2 = (int16_t)((h_buf[1] << 8) | h_buf[0]);
    s_calib.dig_H3 = h_buf[2];
    s_calib.dig_H4 = (int16_t)((h_buf[3] << 4) | (h_buf[4] & 0x0F));
    s_calib.dig_H5 = (int16_t)((h_buf[5] << 4) | (h_buf[4] >> 4));
    s_calib.dig_H6 = (int8_t)h_buf[6];

    return ESP_OK;
}

static float compensate_temperature(int32_t adc_T)
{
    double var1, var2, T;
    var1 = (((double)adc_T) / 16384.0 - ((double)s_calib.dig_T1) / 1024.0) * ((double)s_calib.dig_T2);
    var2 = ((((double)adc_T) / 65536.0 - ((double)s_calib.dig_T1) / 1024.0) *
            (((double)adc_T) / 65536.0 - ((double)s_calib.dig_T1) / 1024.0)) * ((double)s_calib.dig_T3);
    
    s_t_fine = (int32_t)(var1 + var2);
    T = (var1 + var2) / 5120.0;
    return (float)T;
}

static float compensate_pressure(int32_t adc_P) {
    double var1, var2, p;
    var1 = ((double)s_t_fine / 2.0) - 64000.0;
    var2 = var1 * var1 * ((double)s_calib.dig_P6) / 32768.0;
    var2 = var2 + var1 * ((double)s_calib.dig_P5) * 2.0;
    var2 = (var2 / 4.0) + (((double)s_calib.dig_P4) * 65536.0);
    var1 = (((double)s_calib.dig_P3) * var1 * var1 / 524288.0 + ((double)s_calib.dig_P2) * var1) / 524288.0;
    var1 = (1.0 + var1 / 32768.0) * ((double)s_calib.dig_P1);

    if (var1 == 0.0) return 0.0f;

    p = 1048576.0 - (double)adc_P;
    p = (p - (var2 / 4096.0)) * 6250.0 / var1;
    var1 = ((double)s_calib.dig_P9) * p * p / 2147483648.0;
    var2 = p * ((double)s_calib.dig_P8) / 32768.0;
    p = p + (var1 + var2 + ((double)s_calib.dig_P7)) / 16.0;

    return (float)(p / 100.0);
}

static float compensate_humidity(int32_t adc_H)
{
    double var_H;

    var_H = (((double)s_t_fine) - 76800.0);
    var_H = (adc_H - (((double)s_calib.dig_H4) * 64.0 + ((double)s_calib.dig_H5) / 16384.0 * var_H)) *
            (((double)s_calib.dig_H2) / 65536.0 * (1.0 + ((double)s_calib.dig_H6) / 67108864.0 * var_H *
            (1.0 + ((double)s_calib.dig_H5) / 67108864.0 * var_H)));
    var_H = var_H * (1.0 - ((double)s_calib.dig_H1) * var_H / 524288.0);

    if (var_H > 100.0) {
        var_H = 100.0;
    } else if (var_H < 0.0) {
        var_H = 0.0;
    }

    return (float)var_H;
}

esp_err_t bme280_init(i2c_master_dev_handle_t dev_handle)
{
    uint8_t chip_id = 0;
    esp_err_t err = bme280_read_reg(dev_handle, BME280_REG_CHIP_ID, &chip_id, 1);
    if (err != ESP_OK || chip_id != BME280_CHIP_ID_VAL) {
        ESP_LOGE(TAG, "Error locating BME280 (Chip ID: 0x%02X)", chip_id);
        return ESP_ERR_INVALID_VERSION;
    }

    err = bme280_read_calibration(dev_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Unable to read calibrating data");
        return err;
    }

    err = bme280_write_reg(dev_handle, BME280_REG_CTRL_HUM, 0x01);
    if (err != ESP_OK) return err;

    err = bme280_write_reg(dev_handle, BME280_REG_CTRL_MEAS, 0x57);
    if (err != ESP_OK) return err;

    err = bme280_write_reg(dev_handle, BME280_REG_CONFIG, 0x10);
    if (err != ESP_OK) return err;

    ESP_LOGI(TAG, "BME280 configured successfully");
    return ESP_OK;
}

esp_err_t bme280_read_data(i2c_master_dev_handle_t dev_handle, float *temperature, float *pressure, float *humidity)
{
    uint8_t raw[8];

    esp_err_t err = bme280_read_reg(dev_handle, BME280_REG_PRESS_MSB, raw, 8);
    if (err != ESP_OK) return err;

    int32_t adc_P = (raw[0] << 12) | (raw[1] << 4) | (raw[2] >> 4);
    int32_t adc_T = (raw[3] << 12) | (raw[4] << 4) | (raw[5] >> 4);
    int32_t adc_H = (raw[6] << 8) | raw[7];

    float temp = compensate_temperature(adc_T);

    if (temperature) *temperature = temp;
    if (pressure) *pressure = compensate_pressure(adc_P);
    if (humidity) *humidity = compensate_humidity(adc_H);

    return ESP_OK;
}