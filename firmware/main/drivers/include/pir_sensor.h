#ifndef PIR_SENSOR_H
#define PIR_SENSOR_H

#include "esp_err.h"
#include <stdint.h> // For uint16_t
#include <stdbool.h> // For bool

// Define zone enumeration
typedef enum {
    PIR_ZONE_FAR = 0,
    PIR_ZONE_NEAR = 1,
} pir_zone_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Callback function type for PIR sensor events.
 *
 * @param endpoint_id The Matter endpoint ID associated with this sensor.
 * @param occupancy True if occupied, false if not.
 * @param user_data User data pointer passed during initialization.
 */
typedef void (*pir_sensor_event_cb_t)(uint16_t endpoint_id, bool occupancy, void *user_data);

/**
 * @brief Configuration structure for the PIR sensor.
 */
typedef struct {
    pir_sensor_event_cb_t cb;       /**< Callback function to report occupancy changes. */
    uint16_t endpoint_id;         /**< Matter endpoint ID for this sensor. */
    void *user_data;              /**< Optional user data for the callback. */
    pir_zone_t zone;              /**< Which zone this sensor represents (far or near). */
    int gpio_pin;                 /**< GPIO pin number for this PIR sensor. */
} pir_sensor_config_t;

/**
 * @brief Initialize a PIR motion sensor instance
 *
 * Configures the GPIO pin for the PIR sensor and sets up interrupt handling.
 * Can be called multiple times to initialize multiple PIR sensors.
 *
 * @param config Pointer to the PIR sensor configuration structure.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t pir_sensor_init(const pir_sensor_config_t *config);

#ifdef __cplusplus
}
#endif

#endif // PIR_SENSOR_H
