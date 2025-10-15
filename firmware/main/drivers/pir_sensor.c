#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "pir_sensor.h"

// ESP IDF timer includes
#include "esp_timer.h"

// Forward declaration for the C-callable function in app_main.cpp
extern uint16_t get_pir_unoccupied_delay_seconds(uint16_t endpoint_id);

static const char *TAG = "pir_sensor";

#define PIR_MAX_INSTANCES 2

// Per-instance state
typedef struct {
    bool occupied;
    bool initialized;
    pir_sensor_config_t cfg;
    esp_timer_handle_t unocc_timer;
} pir_instance_t;

static pir_instance_t s_instances[PIR_MAX_INSTANCES];
static QueueHandle_t gpio_evt_queue = NULL;
static TaskHandle_t pir_task_handle = NULL;
static bool isr_service_installed = false;

// Forward declarations
static void IRAM_ATTR gpio_isr_handler(void* arg);
static void pir_sensor_task(void* arg);
static void unoccupied_timer_callback(void* arg);
static int find_instance_by_gpio(int gpio);

static int find_instance_by_gpio(int gpio)
{
    for (int i = 0; i < PIR_MAX_INSTANCES; i++) {
        if (s_instances[i].initialized && s_instances[i].cfg.gpio_pin == gpio) {
            return i;
        }
    }
    return -1;
}

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void unoccupied_timer_callback(void* arg)
{
    int idx = (int)(intptr_t)arg;
    
    if (idx < 0 || idx >= PIR_MAX_INSTANCES || !s_instances[idx].initialized) {
        ESP_LOGE(TAG, "Invalid instance index in timer callback: %d", idx);
        return;
    }
    
    ESP_LOGI(TAG, "Unoccupied timer expired for zone %d (GPIO %d). Setting state to UNOCCUPIED.",
             s_instances[idx].cfg.zone, s_instances[idx].cfg.gpio_pin);
    
    s_instances[idx].occupied = false;
    
    if (s_instances[idx].cfg.cb) {
        s_instances[idx].cfg.cb(s_instances[idx].cfg.endpoint_id, false, s_instances[idx].cfg.user_data);
    }
}

static void pir_sensor_task(void* arg)
{
    uint32_t io_num;
    
    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            int idx = find_instance_by_gpio((int)io_num);
            
            if (idx < 0) {
                ESP_LOGW(TAG, "Received GPIO event for unregistered GPIO: %lu", io_num);
                continue;
            }
            
            bool current_level = gpio_get_level(io_num);
            const char *zone_name = (s_instances[idx].cfg.zone == PIR_ZONE_FAR) ? "FAR" : "NEAR";
            
            ESP_LOGD(TAG, "GPIO[%lu] (%s zone) intr, val: %d. Current occupancy: %s", 
                     io_num, zone_name, current_level, 
                     s_instances[idx].occupied ? "OCCUPIED" : "UNOCCUPIED");

            if (current_level == 1) { // Motion detected (PIR output HIGH)
                if (!s_instances[idx].occupied) {
                    ESP_LOGI(TAG, "Motion DETECTED in %s zone (GPIO %lu). Setting state to OCCUPIED.", 
                             zone_name, io_num);
                    s_instances[idx].occupied = true;
                    
                    if (s_instances[idx].cfg.cb) {
                        s_instances[idx].cfg.cb(s_instances[idx].cfg.endpoint_id, 
                                              true, 
                                              s_instances[idx].cfg.user_data);
                    }
                }
                
                // (Re)start the unoccupied timer
                if (s_instances[idx].unocc_timer) {
                    uint16_t delay_seconds = get_pir_unoccupied_delay_seconds(s_instances[idx].cfg.endpoint_id);
                    ESP_LOGI(TAG, "%s zone: Using unoccupied delay of %u seconds.", zone_name, delay_seconds);
                    
                    esp_timer_stop(s_instances[idx].unocc_timer);
                    esp_timer_start_once(s_instances[idx].unocc_timer, (uint64_t)delay_seconds * 1000000ULL);
                    ESP_LOGI(TAG, "%s zone: Unoccupied timer (re)started for %u seconds.", zone_name, delay_seconds);
                }
            } else { // Motion stopped (PIR output LOW)
                uint16_t delay_seconds = get_pir_unoccupied_delay_seconds(s_instances[idx].cfg.endpoint_id);
                ESP_LOGI(TAG, "%s zone: PIR output LOW. Occupancy state remains %s until %us timer expires.", 
                         zone_name,
                         s_instances[idx].occupied ? "OCCUPIED" : "UNOCCUPIED", 
                         delay_seconds);
            }
        }
    }
}

esp_err_t pir_sensor_init(const pir_sensor_config_t *config)
{
    if (!config || !config->cb) {
        ESP_LOGE(TAG, "Invalid configuration or callback missing.");
        return ESP_ERR_INVALID_ARG;
    }
    
    // Find a free slot
    int slot = -1;
    for (int i = 0; i < PIR_MAX_INSTANCES; i++) {
        if (!s_instances[i].initialized) {
            slot = i;
            break;
        }
    }
    
    if (slot < 0) {
        ESP_LOGE(TAG, "No free slots available. Maximum %d PIR sensors supported.", PIR_MAX_INSTANCES);
        return ESP_ERR_NO_MEM;
    }
    
    const char *zone_name = (config->zone == PIR_ZONE_FAR) ? "FAR" : "NEAR";
    ESP_LOGI(TAG, "Initializing PIR sensor for %s zone on GPIO %d (slot %d)", 
             zone_name, config->gpio_pin, slot);
    
    // Copy configuration
    s_instances[slot].cfg = *config;
    s_instances[slot].occupied = false;
    
    // Configure GPIO
    gpio_config_t io_conf = {};    
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.pin_bit_mask = (1ULL << config->gpio_pin);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error configuring GPIO %d: %s", config->gpio_pin, esp_err_to_name(ret));
        return ret;
    }
    
    // Create queue on first init
    if (!gpio_evt_queue) {
        gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
        if (!gpio_evt_queue) {
            ESP_LOGE(TAG, "Failed to create GPIO event queue");
            return ESP_FAIL;
        }
    }
    
    // Start task on first init
    if (!pir_task_handle) {
        if (xTaskCreate(pir_sensor_task, "pir_sensor_task", 3072, NULL, 10, &pir_task_handle) != pdPASS) {
            ESP_LOGE(TAG, "Failed to create PIR sensor task");
            return ESP_FAIL;
        }
    }
    
    // Create per-instance timer
    const esp_timer_create_args_t timer_args = {
        .callback = &unoccupied_timer_callback,
        .arg = (void*)(intptr_t)slot,
        .name = zone_name
    };
    
    ret = esp_timer_create(&timer_args, &s_instances[slot].unocc_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create unoccupied timer for %s zone: %s", 
                 zone_name, esp_err_to_name(ret));
        return ret;
    }
    
    // Install ISR service (once)
    if (!isr_service_installed) {
        ret = gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
        if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
            ESP_LOGE(TAG, "Error installing GPIO ISR service: %s", esp_err_to_name(ret));
            return ret;
        }
        if (ret == ESP_ERR_INVALID_STATE) {
            ESP_LOGW(TAG, "GPIO ISR service already installed.");
        }
        isr_service_installed = true;
    }
    
    // Add ISR handler for this GPIO
    ret = gpio_isr_handler_add(config->gpio_pin, gpio_isr_handler, (void*)(intptr_t)config->gpio_pin);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error adding ISR handler for GPIO %d: %s", 
                 config->gpio_pin, esp_err_to_name(ret));
        esp_timer_delete(s_instances[slot].unocc_timer);
        return ret;
    }
    
    s_instances[slot].initialized = true;
    ESP_LOGI(TAG, "%s zone PIR sensor initialized successfully on GPIO %d", 
             zone_name, config->gpio_pin);
    
    return ESP_OK;
}
