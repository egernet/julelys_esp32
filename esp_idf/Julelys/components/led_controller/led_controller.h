#include <stdio.h>
#include <vector>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

struct RgbwColor
{
    uint32_t red;
    uint32_t green;
    uint32_t blue;
    uint32_t white;

    RgbwColor() : red(0), green(0), blue(0), white(0) {}
    RgbwColor(uint32_t r, uint32_t g, uint32_t b, uint32_t w) : red(r), green(g), blue(b), white(w) {}
    RgbwColor(uint32_t brightness) : red(brightness), green(brightness), blue(brightness), white(0) {}
};

class LedController {
private:
    int ledPin;

    SemaphoreHandle_t bufferMutex;
    std::vector<std::vector<RgbwColor>> frontBuffer;
    std::vector<std::vector<RgbwColor>> backBuffer;

    void configureLed(int pin, uint32_t leds);
    void changeChannel(int toChannel);

    static void ledSequenceTask(void *pvParameter);
public:
    int matrixWidth;
    int matrixHeight;

    bool imageHaveChange = false;
    int updateInterval;

    LedController(int pin, int width, int height);

    void setPixel(uint32_t row, uint32_t col, uint32_t red, uint32_t green, uint32_t blue, uint32_t white);
    void setPixel(uint32_t row, uint32_t col, RgbwColor color);

    void swapBuffers();
    void updateLedTask(void *param);
    void refresh();

    void startupLoopTask();

    void clean();
};

#endif /* LED_CONTROLLER_H */