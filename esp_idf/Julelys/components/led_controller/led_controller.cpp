#include "led_controller.h"
#include "led_strip.h"

#include "freertos/task.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "esp_system.h"
#include "esp_log.h"

#include "driver/gpio.h"

#include <algorithm>

const char TAG[] = "Jylelys.LED";
led_strip_handle_t led_strip;

static const int MAX_FRAME_QUEUE = 3;

std::vector<std::vector<RgbwColor>> image;

std::vector<std::vector<RgbwColor>> initializeMatrix(int rows, int cols) {
    std::vector<std::vector<RgbwColor>> matrix(rows, std::vector<RgbwColor>(cols, RgbwColor(0)));
    return matrix;
}

LedController::LedController(int pin, int width, int height) : ledPin(pin), matrixWidth(width), matrixHeight(height) {
    configureLed(pin, (uint32_t)matrixHeight);
    frameQueue = xQueueCreate(MAX_FRAME_QUEUE, sizeof(std::vector<std::vector<RgbwColor>>));
}

void LedController::configureLed(int pin, uint32_t leds) {
    ESP_LOGI(TAG, "Configured addressable LED");
    ESP_LOGI(TAG, "For pin: %d, with width: %d and height: %d", pin, matrixWidth, matrixHeight);
    
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
    gpio_reset_pin(GPIO_NUM_5);
    gpio_reset_pin(GPIO_NUM_4);
    gpio_set_direction(GPIO_NUM_8, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_NUM_5, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);

    RgbwColor color(255, 0, 0, 0);

    for(int row=0; row<matrixWidth; row++) {
        for(int col=0; col<matrixHeight; col++) {
            image[row][col] = color;
        }
    }

    imageHaveChange = true;
}

void LedController::setPixel(uint32_t row, uint32_t col, uint32_t red, uint32_t green, uint32_t blue, uint32_t white) {
    RgbwColor color(red, green, blue, white);
    setPixel(row, col, color);
}

void LedController::setPixel(uint32_t row, uint32_t col, RgbwColor color) {
    image[row][col] = color;
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

bool LedController::pushFrame(const std::vector<std::vector<RgbwColor>>& newFrame) {
    std::vector<std::vector<RgbwColor>>* frameCopy = new std::vector<std::vector<RgbwColor>>(newFrame);

    if (uxQueueSpacesAvailable(frameQueue) > 0) {
        return xQueueSend(frameQueue, &frameCopy, 0) == pdTRUE;
    } else {
        delete frameCopy; // Drop frame hvis der ikke er plads
        return false;
    }
}

void LedController::changeChannel(int toChannel) {
  gpio_set_level(GPIO_NUM_4, (toChannel >> 0) & 1);
  gpio_set_level(GPIO_NUM_5, (toChannel >> 1) & 1);
  gpio_set_level(GPIO_NUM_8, (toChannel >> 2) & 1);
}

void LedController::updateLedTask(void *param) {
    LedController *controller = static_cast<LedController*>(param);
    std::vector<std::vector<RgbwColor>>* frame = nullptr;
    if (xQueueReceive(controller->frameQueue, &frame, 0) == pdTRUE && frame != nullptr) {
        controller->isReading = true;
        for (int r = 0; r < controller->matrixWidth; r++) {
            controller->changeChannel(r);
            for (int c = 0; c < controller->matrixHeight; c++) {
                const RgbwColor& color = (*frame)[r][c];
                led_strip_set_pixel_rgbw(led_strip, c, color.red, color.green, color.blue, color.white);
            }
            led_strip_refresh(led_strip);
            vTaskDelay(controller->updateInterval / portTICK_PERIOD_MS);
        }
        delete frame;
        controller->isReading = false;
    } else {
        vTaskDelay(controller->updateInterval / portTICK_PERIOD_MS);
    }
}

void LedController::ledSequenceTask(void *pvParameter) {
    LedController *ledController = static_cast<LedController*>(pvParameter);

    while (1) {
        ledController->updateLedTask(pvParameter);
        vTaskDelay(33 / portTICK_PERIOD_MS);
    }
    vTaskDelete( NULL );
}

void LedController::startupLoopTask() {
    xTaskCreate(ledSequenceTask, "led_sequence_task", 2048, this, 5, NULL);
}

void LedController::clean() {
    RgbwColor color(255, 0, 0, 0);

    for(int row=0; row<matrixWidth; row++) {
        for(int col=0; col<matrixHeight; col++) {
            image[row][col] = color;
        }
    }

    imageHaveChange = true;
}