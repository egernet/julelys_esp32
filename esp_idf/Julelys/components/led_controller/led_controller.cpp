#include "led_controller.h"
#include "led_strip.h"

#include "freertos/task.h"

#include "esp_system.h"
#include "esp_log.h"

#include "driver/gpio.h"

#include <algorithm>

#include "sdkconfig.h"

#if CONFIG_JULELYS_LED_TYPE_WS2812B
static const led_model_t LED_MODEL = LED_MODEL_WS2812;
static const led_pixel_format_t LED_PIXEL_FORMAT = LED_PIXEL_FORMAT_GRB;
static const int LED_BITS_PER_PIXEL = 24;
#else
static const led_model_t LED_MODEL = LED_MODEL_SK6812;
static const led_pixel_format_t LED_PIXEL_FORMAT = LED_PIXEL_FORMAT_GRBW;
static const int LED_BITS_PER_PIXEL = 32;
#endif

const char TAG[] = "Julelys.LED";
led_strip_handle_t led_strip;

std::vector<std::vector<RgbwColor>> initializeMatrix(int rows, int cols) {
    std::vector<std::vector<RgbwColor>> matrix(rows, std::vector<RgbwColor>(cols, RgbwColor(0, 0, 0, 0)));
    return matrix;
}

LedController::LedController(int pin, int width, int height) : ledPin(pin), matrixWidth(width), matrixHeight(height) {
    bufferMutex = xSemaphoreCreateMutex();
    configureLed(pin, (uint32_t)matrixHeight);
}

void LedController::configureLed(int pin, uint32_t leds) {
    ESP_LOGI(TAG, "Configured addressable LED");
    ESP_LOGI(TAG, "For pin: %d, with width: %d and height: %d", pin, matrixWidth, matrixHeight);
    ESP_LOGI(TAG, "LED type: %s", LED_BITS_PER_PIXEL == 32 ? "SK6812 RGBW" : "WS2812B RGB");

    frontBuffer = initializeMatrix(matrixWidth, matrixHeight);
    backBuffer = initializeMatrix(matrixWidth, matrixHeight);

    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = pin,
        .max_leds = leds,
        .led_pixel_format = LED_PIXEL_FORMAT,
        .led_model = LED_MODEL,
    };

    int resolution_hz = 10 * 1000 * 1000; // 10MHz

    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = (uint32_t)resolution_hz
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));

    double ledBits = (double)(LED_BITS_PER_PIXEL * leds);
    updateInterval = (int)((ledBits / resolution_hz) * 20000);

    ESP_LOGI(TAG, "Sleep time: %d", updateInterval);

    gpio_reset_pin(GPIO_NUM_8);
    gpio_reset_pin(GPIO_NUM_5);
    gpio_reset_pin(GPIO_NUM_4);
    gpio_set_direction(GPIO_NUM_8, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_NUM_5, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);

    // Initialize both buffers with red
    RgbwColor color(255, 0, 0, 0);
    for (int row = 0; row < matrixWidth; row++) {
        for (int col = 0; col < matrixHeight; col++) {
            frontBuffer[row][col] = color;
            backBuffer[row][col] = color;
        }
    }

    imageHaveChange = true;
}

void LedController::setPixel(uint32_t row, uint32_t col, uint32_t red, uint32_t green, uint32_t blue, uint32_t white) {
    RgbwColor color(red, green, blue, white);
    setPixel(row, col, color);
}

void LedController::setPixel(uint32_t row, uint32_t col, RgbwColor color) {
    // Write to back buffer (no mutex needed - only SPI task writes here)
    backBuffer[row][col] = color;
}

void LedController::swapBuffers() {
    xSemaphoreTake(bufferMutex, portMAX_DELAY);
    std::swap(frontBuffer, backBuffer);
    imageHaveChange = true;
    xSemaphoreGive(bufferMutex);
}

void LedController::refresh() {
    xSemaphoreTake(bufferMutex, portMAX_DELAY);

    for (int r = 0; r < matrixWidth; r++) {
        changeChannel(r);

        for (int c = 0; c < matrixHeight; c++) {
            RgbwColor color = frontBuffer[r][c];
#if CONFIG_JULELYS_LED_TYPE_WS2812B
            // No white diode, so mix white into the color channels
            uint32_t red = std::min<uint32_t>(color.red + color.white, 255);
            uint32_t green = std::min<uint32_t>(color.green + color.white, 255);
            uint32_t blue = std::min<uint32_t>(color.blue + color.white, 255);
#if CONFIG_JULELYS_LED_COLOR_ORDER_RGB
            // The driver sends GRB, so swap red and green for RGB strings
            std::swap(red, green);
#endif
            led_strip_set_pixel(led_strip, c, red, green, blue);
#else
            led_strip_set_pixel_rgbw(led_strip, c, color.red, color.green, color.blue, color.white);
#endif
        }

        led_strip_refresh(led_strip);

        // Release mutex during LED timing delay to allow buffer swap
        xSemaphoreGive(bufferMutex);
        vTaskDelay(updateInterval / portTICK_PERIOD_MS);
        xSemaphoreTake(bufferMutex, portMAX_DELAY);
    }

    xSemaphoreGive(bufferMutex);
}

void LedController::changeChannel(int toChannel) {
    gpio_set_level(GPIO_NUM_4, (toChannel >> 0) & 1);
    gpio_set_level(GPIO_NUM_5, (toChannel >> 1) & 1);
    gpio_set_level(GPIO_NUM_8, (toChannel >> 2) & 1);
}

void LedController::updateLedTask(void *param) {
    if (imageHaveChange == false) {
        vTaskDelay(1);  // Yield, men vent ikke længe
    } else {
        refresh();
        imageHaveChange = false;
    }
}

void LedController::ledSequenceTask(void *pvParameter) {
    LedController *ledController = static_cast<LedController*>(pvParameter);

    while (1) {
        ledController->updateLedTask(pvParameter);
        vTaskDelay(1);  // Minimal yield i stedet for 33ms
    }
    vTaskDelete(NULL);
}

void LedController::startupLoopTask() {
    xTaskCreate(ledSequenceTask, "led_sequence_task", 2048, this, 5, NULL);
}

void LedController::clean() {
    RgbwColor color(255, 0, 0, 0);

    for (int row = 0; row < matrixWidth; row++) {
        for (int col = 0; col < matrixHeight; col++) {
            backBuffer[row][col] = color;
        }
    }

    swapBuffers();
}
