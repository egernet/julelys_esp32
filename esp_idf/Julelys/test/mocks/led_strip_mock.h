#pragma once

#include <stdint.h>
#include "esp_types.h"

// LED strip handle
typedef void* led_strip_handle_t;

// LED pixel format
typedef int led_pixel_format_t;
#define LED_PIXEL_FORMAT_GRBW 1

// LED model
typedef int led_model_t;
#define LED_MODEL_SK6812 1

// LED strip config
typedef struct {
    int strip_gpio_num;
    uint32_t max_leds;
    led_pixel_format_t led_pixel_format;
    led_model_t led_model;
} led_strip_config_t;

// RMT config
typedef struct {
    uint32_t resolution_hz;
} led_strip_rmt_config_t;

// Mock tracking
struct LedStripMockState {
    int pixels_set;
    int refreshes;
    uint8_t pixels[512][4]; // Store up to 512 pixels [index][r,g,b,w]
};

// Global mock state
extern LedStripMockState g_led_strip_mock;

// Mock functions
inline esp_err_t led_strip_new_rmt_device(const led_strip_config_t* config,
                                           const led_strip_rmt_config_t* rmt_config,
                                           led_strip_handle_t* handle) {
    (void)config; (void)rmt_config;
    *handle = (void*)1;
    return ESP_OK;
}

inline esp_err_t led_strip_set_pixel_rgbw(led_strip_handle_t handle, int index,
                                           uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
    (void)handle;
    if (index >= 0 && index < 512) {
        g_led_strip_mock.pixels[index][0] = r;
        g_led_strip_mock.pixels[index][1] = g;
        g_led_strip_mock.pixels[index][2] = b;
        g_led_strip_mock.pixels[index][3] = w;
    }
    g_led_strip_mock.pixels_set++;
    return ESP_OK;
}

inline esp_err_t led_strip_refresh(led_strip_handle_t handle) {
    (void)handle;
    g_led_strip_mock.refreshes++;
    return ESP_OK;
}

inline esp_err_t led_strip_clear(led_strip_handle_t handle) {
    (void)handle;
    return ESP_OK;
}
