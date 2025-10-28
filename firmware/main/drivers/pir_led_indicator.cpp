/*
 * PIR LED Indicator Driver
 * Uses ESP32 LEDC peripheral for PWM control of LED brightness
 */

#include "pir_led_indicator.h"
#include <driver/ledc.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char *TAG = "led_indicator";

// LEDC configuration
#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT  // 13-bit resolution (0-8191)
#define LEDC_FREQUENCY          (5000)             // 5 kHz

// Brightness levels (0-8191 for 13-bit)
#define BRIGHTNESS_DIM          (819)   // ~10% duty cycle
#define BRIGHTNESS_BRIGHT       (8191)  // 100% duty cycle
#define BRIGHTNESS_OFF          (0)     // Off

// Blink timing
#define BLINK_DURATION_MS       (150)

static int s_led_gpio = -1;
static bool s_initialized = false;

static esp_err_t set_brightness(uint32_t duty)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty);
    if (err != ESP_OK) {
        return err;
    }

    return ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}

// Task that executes blink pattern then stays bright
static void blink_task(void *pvParameters)
{
    int blink_count = (int)(intptr_t)pvParameters;

    for (int i = 0; i < blink_count; i++) {
        set_brightness(BRIGHTNESS_BRIGHT);
        vTaskDelay(pdMS_TO_TICKS(BLINK_DURATION_MS));
        
        set_brightness(BRIGHTNESS_OFF);
        vTaskDelay(pdMS_TO_TICKS(BLINK_DURATION_MS));
    }

    // After blinking, stay bright
    set_brightness(BRIGHTNESS_BRIGHT);

    // Task complete, delete self
    vTaskDelete(NULL);
}

esp_err_t pir_led_indicator_init(int gpio_num)
{
    if (s_initialized) {
        ESP_LOGW(TAG, "LED indicator already initialized");
        return ESP_OK;
    }

    // Configure LEDC timer
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .duty_resolution  = LEDC_DUTY_RES,
        .timer_num        = LEDC_TIMER,
        .freq_hz          = LEDC_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    esp_err_t err = ledc_timer_config(&ledc_timer);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LEDC timer: %s", esp_err_to_name(err));
        return err;
    }

    // Configure LEDC channel
    ledc_channel_config_t ledc_channel = {
        .gpio_num       = gpio_num,
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = LEDC_TIMER,
        .duty           = BRIGHTNESS_DIM,  // Start dim
        .hpoint         = 0,
        .flags          = {
            .output_invert = 0
        }
    };
    err = ledc_channel_config(&ledc_channel);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LEDC channel: %s", esp_err_to_name(err));
        return err;
    }

    s_led_gpio = gpio_num;
    s_initialized = true;

    ESP_LOGI(TAG, "LED indicator initialized on GPIO %d", gpio_num);
    return ESP_OK;
}

esp_err_t pir_led_indicator_set_dim(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    return set_brightness(BRIGHTNESS_DIM);
}

esp_err_t pir_led_indicator_set_bright(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    return set_brightness(BRIGHTNESS_BRIGHT);
}

esp_err_t pir_led_indicator_blink_far(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    // Create task to execute 2 blinks then stay bright
    BaseType_t ret = xTaskCreate(
        blink_task,
        "led_blink_far",
        2048,
        (void *)(intptr_t)2,  // 2 blinks for far zone
        5,
        NULL
    );

    return (ret == pdPASS) ? ESP_OK : ESP_FAIL;
}

esp_err_t pir_led_indicator_blink_near(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    // Create task to execute 4 blinks then stay bright
    BaseType_t ret = xTaskCreate(
        blink_task,
        "led_blink_near",
        2048,
        (void *)(intptr_t)4,  // 4 blinks for near zone
        5,
        NULL
    );

    return (ret == pdPASS) ? ESP_OK : ESP_FAIL;
}

esp_err_t pir_led_indicator_deinit(void)
{
    if (!s_initialized) {
        return ESP_OK;
    }

    // Turn off LED
    set_brightness(BRIGHTNESS_OFF);

    // Stop LEDC channel
    ledc_stop(LEDC_MODE, LEDC_CHANNEL, 0);

    s_initialized = false;
    s_led_gpio = -1;

    ESP_LOGI(TAG, "LED indicator deinitialized");
    return ESP_OK;
}

