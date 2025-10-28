/*
 * PIR LED Indicator Driver
 * Provides zone-specific visual feedback for occupancy detection
 */

#pragma once

#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize LED indicator
 * @param gpio_num GPIO pin number for LED control
 * @return ESP_OK on success
 */
esp_err_t pir_led_indicator_init(int gpio_num);

/**
 * @brief Set LED to dim state (idle/unoccupied)
 * @return ESP_OK on success
 */
esp_err_t pir_led_indicator_set_dim(void);

/**
 * @brief Far zone pattern: 2 blinks then bright
 * @return ESP_OK on success
 */
esp_err_t pir_led_indicator_blink_far(void);

/**
 * @brief Near zone pattern: 4 blinks then bright
 * @return ESP_OK on success
 */
esp_err_t pir_led_indicator_blink_near(void);

/**
 * @brief Set LED to full brightness (occupied state)
 * @return ESP_OK on success
 */
esp_err_t pir_led_indicator_set_bright(void);

/**
 * @brief Deinitialize and free resources
 * @return ESP_OK on success
 */
esp_err_t pir_led_indicator_deinit(void);

#ifdef __cplusplus
}
#endif

