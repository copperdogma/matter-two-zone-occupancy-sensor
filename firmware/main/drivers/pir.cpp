/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <array>

#include <driver/gpio.h>
#include <esp_attr.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_timer.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include <drivers/pir.h>

namespace {
constexpr size_t kMaxSensors = 4;
constexpr int kIsrFlags = ESP_INTR_FLAG_LEVEL1;
constexpr gpio_num_t kInvalidGpio = GPIO_NUM_NC;
constexpr uint32_t kDefaultUnoccupiedDelayMs = 10'000;
const char *TAG = "pir";

struct pir_sensor_ctx_t {
    pir_sensor_config_t config{};
    gpio_num_t gpio_num = kInvalidGpio;
    bool is_initialized = false;
    bool last_level = false;
    bool occupancy = false;
    esp_timer_handle_t unoccupied_timer = nullptr;
};

struct pir_event_t {
    pir_sensor_ctx_t *ctx = nullptr;
    bool level = false;
};

std::array<pir_sensor_ctx_t, kMaxSensors> s_sensors{};
bool s_isr_service_installed = false;
QueueHandle_t s_event_queue = nullptr;
TaskHandle_t s_event_task = nullptr;

pir_sensor_ctx_t *find_ctx(gpio_num_t gpio)
{
    for (auto &ctx : s_sensors) {
        if (ctx.is_initialized && ctx.gpio_num == gpio) {
            return &ctx;
        }
    }
    return nullptr;
}

pir_sensor_ctx_t *allocate_ctx()
{
    for (auto &ctx : s_sensors) {
        if (!ctx.is_initialized) {
            return &ctx;
        }
    }
    return nullptr;
}

void report_occupancy(pir_sensor_ctx_t *ctx, bool occupied)
{
    if (ctx == nullptr || ctx->config.cb == nullptr) {
        return;
    }
    ctx->config.cb(ctx->config.endpoint_id, occupied, ctx->config.user_data);
}

void unoccupied_timer_cb(void *arg)
{
    auto *ctx = static_cast<pir_sensor_ctx_t *>(arg);
    if (ctx == nullptr) {
        return;
    }
    if (ctx->occupancy) {
        ctx->occupancy = false;
        report_occupancy(ctx, false);
    }
}

void handle_level_change(pir_sensor_ctx_t *ctx, bool level)
{
    if (ctx == nullptr) {
        return;
    }

    if (level) {
        if (!ctx->occupancy) {
            ctx->occupancy = true;
            report_occupancy(ctx, true);
        }
        if (ctx->unoccupied_timer) {
            esp_timer_stop(ctx->unoccupied_timer);
            esp_timer_start_once(ctx->unoccupied_timer, kDefaultUnoccupiedDelayMs * 1000ULL);
        }
    } else {
        if (ctx->unoccupied_timer) {
            esp_timer_stop(ctx->unoccupied_timer);
            esp_timer_start_once(ctx->unoccupied_timer, kDefaultUnoccupiedDelayMs * 1000ULL);
        }
    }
}

void pir_event_task(void *arg)
{
    pir_event_t evt{};
    while (xQueueReceive(s_event_queue, &evt, portMAX_DELAY) == pdTRUE) {
        if (evt.ctx == nullptr) {
            continue;
        }
        if (evt.level != evt.ctx->last_level) {
            evt.ctx->last_level = evt.level;
            handle_level_change(evt.ctx, evt.level);
        }
    }
}
} // namespace

static void IRAM_ATTR pir_gpio_handler(void *arg)
{
    auto *ctx = static_cast<pir_sensor_ctx_t *>(arg);
    if (ctx == nullptr || !ctx->is_initialized || s_event_queue == nullptr) {
        return;
    }

    const bool new_level = gpio_get_level(ctx->gpio_num);
    pir_event_t evt{ctx, new_level};
    BaseType_t higher_priority_task_woken = pdFALSE;
    xQueueSendFromISR(s_event_queue, &evt, &higher_priority_task_woken);
    if (higher_priority_task_woken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

esp_err_t pir_sensor_init(const pir_sensor_config_t *config)
{
    if (config == nullptr || config->cb == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    const gpio_num_t gpio = config->gpio_num;
    if (gpio == kInvalidGpio || !GPIO_IS_VALID_GPIO(gpio)) {
        ESP_LOGE(TAG, "Invalid GPIO specified for PIR sensor: %d", static_cast<int>(gpio));
        return ESP_ERR_INVALID_ARG;
    }

    if (find_ctx(gpio) != nullptr) {
        ESP_LOGE(TAG, "GPIO %d already registered for a PIR sensor", static_cast<int>(gpio));
        return ESP_ERR_INVALID_STATE;
    }

    pir_sensor_ctx_t *ctx = allocate_ctx();
    if (ctx == nullptr) {
        ESP_LOGE(TAG, "No available PIR sensor slots");
        return ESP_ERR_NO_MEM;
    }

    esp_err_t err = gpio_reset_pin(gpio);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to reset GPIO %d: %s", static_cast<int>(gpio), esp_err_to_name(err));
        return err;
    }

    gpio_config_t io_conf{};
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.pin_bit_mask = (1ULL << static_cast<uint32_t>(gpio));
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;

    err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO %d: %s", static_cast<int>(gpio), esp_err_to_name(err));
        return err;
    }

    err = gpio_set_pull_mode(gpio, GPIO_PULLDOWN_ONLY);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set pull mode for GPIO %d: %s", static_cast<int>(gpio), esp_err_to_name(err));
        return err;
    }

    if (!s_isr_service_installed) {
        err = gpio_install_isr_service(kIsrFlags);
        if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
            ESP_LOGE(TAG, "Failed to install GPIO ISR service: %s", esp_err_to_name(err));
            return err;
        }
        s_isr_service_installed = true;
    }

    if (s_event_queue == nullptr) {
        s_event_queue = xQueueCreate(16, sizeof(pir_event_t));
        if (s_event_queue == nullptr) {
            ESP_LOGE(TAG, "Failed to create PIR event queue");
            return ESP_ERR_NO_MEM;
        }
    }

    if (s_event_task == nullptr) {
        if (xTaskCreatePinnedToCore(pir_event_task,
                                    "pir_event_task",
                                    3072,
                                    nullptr,
                                    configMAX_PRIORITIES - 2,
                                    &s_event_task,
                                    tskNO_AFFINITY) != pdPASS) {
            ESP_LOGE(TAG, "Failed to create PIR event task");
            return ESP_ERR_NO_MEM;
        }
    }

    ctx->config = *config;
    ctx->gpio_num = gpio;
    ctx->last_level = gpio_get_level(gpio);
    ctx->occupancy = ctx->last_level;
    ctx->is_initialized = true;

    if (ctx->unoccupied_timer == nullptr) {
        const esp_timer_create_args_t timer_args = {
            .callback = &unoccupied_timer_cb,
            .arg = ctx,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "pir-unocc",
            .skip_unhandled_events = false,
        };
        err = esp_timer_create(&timer_args, &ctx->unoccupied_timer);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to create timer for GPIO %d: %s", static_cast<int>(gpio), esp_err_to_name(err));
            ctx->is_initialized = false;
            return err;
        }
    }

    err = gpio_isr_handler_add(gpio, pir_gpio_handler, ctx);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add ISR handler for GPIO %d: %s", static_cast<int>(gpio), esp_err_to_name(err));
        ctx->is_initialized = false;
        esp_timer_delete(ctx->unoccupied_timer);
        ctx->unoccupied_timer = nullptr;
        return err;
    }

    err = gpio_intr_enable(gpio);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable interrupts for GPIO %d: %s", static_cast<int>(gpio), esp_err_to_name(err));
        gpio_isr_handler_remove(gpio);
        ctx->is_initialized = false;
        esp_timer_delete(ctx->unoccupied_timer);
        ctx->unoccupied_timer = nullptr;
        return err;
    }

    ESP_LOGI(TAG, "Registered PIR sensor on GPIO %d (endpoint %u)", static_cast<int>(gpio), config->endpoint_id);

    // Prime initial state so controller reflects current level.
    pir_event_t initial_evt{ctx, ctx->last_level};
    xQueueSend(s_event_queue, &initial_evt, 0);

    return ESP_OK;
}
