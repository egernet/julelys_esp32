#include "js_controller.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "elk.h"

#include "led_strip.h"

#include <esp_timer.h>

#include "led_controller.h"

char jsBuf[16384];
extern LedController *ledController;

JSController::JSController(LedController *controller) {
    ledController = controller;
}

void delay(int time) {
    vTaskDelay(time / portTICK_PERIOD_MS);
}

jsval_t jsDelay(struct js *js, jsval_t *args, int nargs) {
    delay(js_getnum(args[0]));
    
    return js_mknum(0);
}

jsval_t setPixelColor(struct js *js, jsval_t *args, int nargs) {
    
    uint32_t red = (uint32_t)js_getnum(args[0]);
    uint32_t green = (uint32_t)js_getnum(args[1]);
    uint32_t blue = (uint32_t)js_getnum(args[2]);
    uint32_t white = (uint32_t)js_getnum(args[3]);
    uint32_t x = (uint32_t)js_getnum(args[4]);
    uint32_t y = (uint32_t)js_getnum(args[5]);

    ledController->setPixel(x, red, green, blue, white);

    return js_mknum(0);
}

jsval_t updatePixels(struct js *js, jsval_t *args, int nargs) {
    ledController->refresh();
    
    return js_mknum(0);
}

void JSController::runCode(const char *code) {
    stop = false;
    double matrixHeight = 55;
    double matrixWidth = 1;
    
    long startTime = (long)(esp_timer_get_time() / 1000);
    long time = startTime;
    
    while (!stop) { 
        long time = (long)(esp_timer_get_time() / 1000);  

        jsEngine = setup(matrixHeight, matrixWidth, startTime, time);
        js_eval(jsEngine,
            code,
            ~0U);
    }
}

struct js *JSController::setup(double matrixHeight, double matrixWidth, long start, long time) { 
    struct js *jsEngine = js_create(jsBuf, sizeof(jsBuf));
    jsval_t global = js_glob(jsEngine);
    jsval_t matrix = js_mkobj(jsEngine);
    jsval_t frame = js_mkobj(jsEngine);

    js_set(jsEngine, global, "delay", js_mkfun(jsDelay));
    js_set(jsEngine, global, "setPixelColor", js_mkfun(setPixelColor));
    js_set(jsEngine, global, "updatePixels", js_mkfun(updatePixels));
    js_set(jsEngine, global, "matrix", matrix);
    js_set(jsEngine, global, "frame", frame);
    
    js_set(jsEngine, matrix, "height", js_mknum(matrixHeight));
    js_set(jsEngine, matrix, "width", js_mknum(matrixWidth));

    js_set(jsEngine, frame, "start", js_mknum(start));
    js_set(jsEngine, frame, "time", js_mknum(time));

    return jsEngine;
}

void JSController::stopRinning() {
    stop = true;
}