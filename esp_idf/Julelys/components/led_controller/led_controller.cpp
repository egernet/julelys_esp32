#include "led_controller.h"
#include "led_strip.h"

#include "esp_system.h"
#include "esp_log.h"

const char TAG[] = "Jylelys.LED";
led_strip_handle_t led_strip;

LedController::LedController(int pin, int width, int height) : ledPin(pin), matrixWidth(width), matrixHeight(height) {
    configureLed(pin, (uint32_t)matrixHeight);
}

void LedController::configureLed(int pin, uint32_t leds) {
    ESP_LOGI(TAG, "Example configured to blink addressable LED!");
    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = pin,
        .max_leds = leds, // at least one LED on board
        .led_pixel_format = LED_PIXEL_FORMAT_GRBW,
        .led_model = LED_MODEL_SK6812,
    };

    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));

    // led_strip_clear(led_strip);
}

void LedController::setPixel(uint32_t index, uint32_t red, uint32_t green, uint32_t blue, uint32_t white) {
    led_strip_set_pixel_rgbw(led_strip, index, red, green, blue, white);
}

void LedController::refresh() {
    led_strip_refresh(led_strip);
}