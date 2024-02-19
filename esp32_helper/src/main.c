#include <stdio.h>
#include <string.h>

#include "esp_adc/adc_oneshot.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "led_strip_rmt.h"
#include "esp_console.h"
#include "led_strip.h"
#include "soc/soc_caps.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#define EXAMPLE_ADC1_CHAN0          ADC_CHANNEL_0
#define EXAMPLE_ADC1_CHAN1          ADC_CHANNEL_1
#define EXAMPLE_ADC_ATTEN           ADC_ATTEN_DB_11

static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle);

static led_strip_handle_t led_strip;

void command_task() {
    while (true) {
        char cmd_buf[64];
        int cmd_buf_index = 0;
        char *argv[8];
        int argc = 0;

        while (true) {
            char c = fgetc(stdin);
            if (c != 255) {
                cmd_buf[cmd_buf_index] = c;
                if (cmd_buf_index < sizeof(cmd_buf) - 1) {
                    cmd_buf_index++;
                }
            } else {
                vTaskDelay(10);
            }

            if (c == '\n') {
                cmd_buf[cmd_buf_index] = '\0';
                break;
            }
        }
        argc = esp_console_split_argv(cmd_buf, argv, 8);

        if (argc == 0) {
            continue;
        }

        if (strcmp(argv[0], "rgb") == 0) {
            if (argc != 4) {
                continue;
            }
            int r = atoi(argv[1]);
            int g = atoi(argv[2]);
            int b = atoi(argv[3]);

            led_strip_set_pixel(led_strip, 0, r, g, b);
            led_strip_refresh(led_strip);
        }
    }
}


void adc_task(void)
{
    //-------------ADC1 Init---------------//
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    //-------------ADC1 Config---------------//
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = EXAMPLE_ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, EXAMPLE_ADC1_CHAN0, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, EXAMPLE_ADC1_CHAN1, &config));

    //-------------ADC1 Calibration Init---------------//
    adc_cali_handle_t adc1_cali_chan0_handle = NULL;
    adc_cali_handle_t adc1_cali_chan1_handle = NULL;
    bool do_calibration1_chan0 = example_adc_calibration_init(ADC_UNIT_1, EXAMPLE_ADC1_CHAN0, EXAMPLE_ADC_ATTEN, &adc1_cali_chan0_handle);
    bool do_calibration1_chan1 = example_adc_calibration_init(ADC_UNIT_1, EXAMPLE_ADC1_CHAN1, EXAMPLE_ADC_ATTEN, &adc1_cali_chan1_handle);
    float volt[2];
    int count = 0;
    while (1) {
        int adc_raw[2];
        int new_volt[2];
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, EXAMPLE_ADC1_CHAN0, &adc_raw[0]));
        if (do_calibration1_chan0) {
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_chan0_handle, adc_raw[0], &new_volt[0]));
        }

        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, EXAMPLE_ADC1_CHAN1, &adc_raw[1]));
        if (do_calibration1_chan1) {
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_chan1_handle, adc_raw[1], &new_volt[1]));
        }

        volt[0] = new_volt[0] / 1000.0 * 0.001 + volt[0] * 0.999;
        volt[1] = new_volt[1] / 1000.0 * 0.001 + volt[1] * 0.999;
        vTaskDelay(pdMS_TO_TICKS(1));

        if (count++ % 100 == 0) {
            printf("adc1 %f\n", volt[0]);
            printf("adc2 %f\n", volt[1]);
        }
    }
}

/*---------------------------------------------------------------
        ADC Calibration
---------------------------------------------------------------*/
static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    *out_handle = handle;
    if (ret == ESP_OK) {
        // ESP_LOGI(TAG, "Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        // ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    } else {
        // ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return calibrated;
}

void app_main() {
    led_strip_config_t strip_config = {
        .strip_gpio_num = 48,
        .max_leds = 1, // at least one LED on board
    };

    led_strip_rmt_config_t rmt_config = {
        .flags.with_dma = false,
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));

    led_strip_clear(led_strip);

    led_strip_set_pixel(led_strip, 0, 128, 128, 128);
    led_strip_refresh(led_strip);

    TaskHandle_t command_task_handle;
    xTaskCreate(command_task, "command_task", 20000, NULL, NULL, &command_task_handle);

    TaskHandle_t adc_task_handle;
    xTaskCreate(adc_task, "adc_task", 20000, NULL, NULL, &adc_task_handle);
}