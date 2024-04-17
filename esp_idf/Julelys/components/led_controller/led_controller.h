#include <stdio.h>

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
    
    void configureLed(int pin, uint32_t leds);
    void changeChannel(int toChannel);
    
public:
    int matrixWidth;
    int matrixHeight;

    LedController(int pin, int width, int height);

    void setPixel(uint32_t row, uint32_t col, uint32_t red, uint32_t green, uint32_t blue, uint32_t white);
    void refresh(int row);
};

#endif /* LED_CONTROLLER_H */