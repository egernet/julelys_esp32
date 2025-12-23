#include "rain_sequence.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_task_wdt.h"

#include "led_controller.h"

extern LedController *ledController;

bool rain_stop = false;

RgbwColor colorWheel(int pos) {
    int r = 0;
    int g = 0;
    int b = 0;
    pos = 255 - pos;

    if ( pos < 85 ) {
        r = 255 - pos * 3;
        g = 0;
        b = pos * 3;
    } else if (pos < 170) {
        pos -= 85;
        r = 0;
        g = pos * 3;
        b = 255 - pos * 3;
    } else {
        pos -= 170;
        r = pos * 3;
        g = 255 - pos * 3;
        b = 0;
    }

    RgbwColor color(r, g, b, 0);

    return color;
}

void setPixel(uint32_t row, uint32_t col, RgbwColor color) {
    ledController->setPixel(row, col, color);
}

void rain_sequence_task(void *pvParameter) {
    int width = ledController->matrixWidth;
    int height = ledController->matrixHeight;
    int updateInterval = 10;//ledController->updateInterval;
    int iterations = 1;

    while (rain_stop == false) {
        for(int i = 0; i < 255 * iterations; i++) {
            for(int y = 0; y < width; y++) {
                for(int x = 0; x < height; x++) {
                    int index = ((x * 255 / height) + i) & 255;
                    RgbwColor showColor = colorWheel( index );
                    setPixel(y, x, showColor);
                }
            }

            ledController->imageHaveChange = true;
            do {
                vTaskDelay(updateInterval / portTICK_PERIOD_MS);
            } while(ledController->isReading);
        }
    }

    vTaskDelete(NULL);
}

void stopRainTask() {
    rain_stop = true;

    ledController->clean();
}

void startupRainTask() {
    rain_stop = false;
    
    xTaskCreate(rain_sequence_task, "rain_sequence_task", 2048, NULL, 5, NULL);
}