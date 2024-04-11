#include <stdio.h>

#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

class LedController {
private:
    int ledPin;
    int matrixWidth;
    int matrixHeight;

    void configureLed(int pin, uint32_t leds);

public:
    LedController(int pin, int width, int height);

    void setPixel(uint32_t index, uint32_t red, uint32_t green, uint32_t blue, uint32_t white);
    void refresh();
};

#endif /* LED_CONTROLLER_H */