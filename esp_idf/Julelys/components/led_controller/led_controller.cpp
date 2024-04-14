#include "led_controller.h"
#include "led_strip.h"

#include "freertos/task.h"

#include "esp_system.h"
#include "esp_log.h"

#include "driver/gpio.h"

#include <algorithm>
#include <vector>

const char TAG[] = "Jylelys.LED";
led_strip_handle_t led_strip;

bool isReading = false;
bool imageHaveChange = false;
int updateInterval;

std::vector<std::vector<RgbwColor>> image;

std::vector<std::vector<RgbwColor>> initializeMatrix(int rows, int cols) {
    std::vector<std::vector<RgbwColor>> matrix(rows, std::vector<RgbwColor>(cols, RgbwColor(0)));
    return matrix;
}

LedController::LedController(int pin, int width, int height) : ledPin(pin), matrixWidth(width), matrixHeight(height) {
    configureLed(pin, (uint32_t)matrixHeight);
}

void LedController::configureLed(int pin, uint32_t leds) {
    ESP_LOGI(TAG, "Example configured to blink addressable LED!");
    
    image = initializeMatrix(matrixWidth, matrixHeight);

    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = pin,
        .max_leds = leds, // at least one LED on board
        .led_pixel_format = LED_PIXEL_FORMAT_GRBW,
        .led_model = LED_MODEL_SK6812,
    };

    int resolution_hz = 10 * 1000 * 1000; // 10MHz
    //int resolution_hz = 800 * 1000; // 800kHz

    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = (uint32_t)resolution_hz
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));

    // led_strip_clear(led_strip);

    double ledBits = (double)(32 * leds);
    updateInterval = (int)((ledBits / resolution_hz) * 20000);

    ESP_LOGI(TAG, "Sleep time: %d", updateInterval);

    gpio_reset_pin(GPIO_NUM_8);
    gpio_reset_pin(GPIO_NUM_7);
    gpio_reset_pin(GPIO_NUM_6);
    gpio_set_direction(GPIO_NUM_8, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_NUM_7, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_NUM_6, GPIO_MODE_OUTPUT);

    RgbwColor color(255, 0, 0, 0);

    for(int row=0; row<matrixWidth; row++) {
        for(int col=0; col<matrixHeight; col++) {
            image[row][col] = color;
        }
    }
}

void LedController::setPixel(uint32_t row, uint32_t col, uint32_t red, uint32_t green, uint32_t blue, uint32_t white) {
    RgbwColor color(red, green, blue, white);
    image[row][col] = color;

    // imageHaveChange = true;
    // do {
    //    vTaskDelay(updateInterval * 10 / portTICK_PERIOD_MS);
    // } while(isReading);
}

void LedController::refresh() {
    for(int r=0; r<matrixWidth; r++) {
        changeChannel(r);

        for(int c=0; c<matrixHeight; c++) {
            RgbwColor color = image[r][c];
            led_strip_set_pixel_rgbw(led_strip, c, color.red, color.green, color.blue, color.white);
        }

        led_strip_refresh(led_strip);
        vTaskDelay(updateInterval / portTICK_PERIOD_MS);
  }
}

void LedController::changeChannel(int toChannel) {
  gpio_set_level(GPIO_NUM_6, (toChannel >> 0) & 1);
  gpio_set_level(GPIO_NUM_7, (toChannel >> 1) & 1);
  gpio_set_level(GPIO_NUM_8, (toChannel >> 2) & 1);
}

void LedController::updateLedTask(void *param) {
    // if (imageHaveChange == false) {
    //     vTaskDelay(updateInterval / portTICK_PERIOD_MS);
    // } else {
        isReading = true;
        refresh(); 
        isReading = false;   
        imageHaveChange = false;         
    // }
}