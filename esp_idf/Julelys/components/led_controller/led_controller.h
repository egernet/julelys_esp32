#include <stdio.h>
#include <vector>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

struct RgbwColor
{
    uint32_t red;
    uint32_t green;
    uint32_t blue;
    uint32_t white;
};

class LedController {
private:
    int ledPin;
    QueueHandle_t frameQueue = nullptr;

    void configureLed(int pin, uint32_t leds);
    void changeChannel(int toChannel);

    static void ledSequenceTask(void *pvParameter);
public:
    int matrixWidth;
    int matrixHeight;

    bool isReading = false;
    bool imageHaveChange = false;
    int updateInterval;

    LedController(int pin, int width, int height);

    void setPixel(uint32_t row, uint32_t col, uint32_t red, uint32_t green, uint32_t blue, uint32_t white);
    void setPixel(uint32_t row, uint32_t col, RgbwColor color);
    
    void updateLedTask(void *param);
    void refresh();

    void startupLoopTask(); 

    void clean();   

    bool pushFrame(const std::vector<std::vector<RgbwColor>>& newFrame);
};

#endif /* LED_CONTROLLER_H */