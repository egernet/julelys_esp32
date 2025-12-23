#pragma once

#include "esp_types.h"

// GPIO mock state
struct GpioMockState {
    int levels[20]; // Track GPIO levels
};

extern GpioMockState g_gpio_mock;

// GPIO functions
inline esp_err_t gpio_reset_pin(gpio_num_t pin) { (void)pin; return ESP_OK; }
inline esp_err_t gpio_set_direction(gpio_num_t pin, gpio_mode_t mode) { (void)pin; (void)mode; return ESP_OK; }
inline esp_err_t gpio_set_level(gpio_num_t pin, int level) {
    if (pin >= 0 && pin < 20) {
        g_gpio_mock.levels[pin] = level;
    }
    return ESP_OK;
}
