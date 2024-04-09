#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"

#include "esp_system.h"
#include "esp_log.h"

#include "sdkconfig.h"

#include "elk.h"

#include <esp_wifi.h>
#include <esp_netif.h>

#include "led_strip.h"

#include <esp_heap_caps.h>

#include "js_string.h"

#include <esp_timer.h>

static const char TAG[] = "main";

#ifndef IP4ADDR_STRLEN_MAX
#define IP4ADDR_STRLEN_MAX  16
#endif

static led_strip_handle_t led_strip;
static uint8_t s_led_state = 0;

char jsBuf[16384];

void delay(int time) {
    vTaskDelay(time / portTICK_PERIOD_MS);
}

jsval_t myDelay(struct js *js, jsval_t *args, int nargs) {
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

    led_strip_set_pixel_rgbw(led_strip, x, red, green, blue, white);

    //led_strip_set_pixel(led_strip, x, red, green, blue);

    return js_mknum(0);
}

jsval_t updatePixels(struct js *js, jsval_t *args, int nargs) {
    // printf("UpdatePixels\n");
    led_strip_refresh(led_strip);
    
    return js_mknum(0);
}

static void blink_led(void)
{
    /* If the addressable LED is enabled */
    if (s_led_state) {
        /* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
        for (uint32_t i=0; i < 7 ; i++) {
            led_strip_set_pixel_rgbw(led_strip, i, 255, 0, 0, 0);
        }
        /* Refresh the strip to send data */
        led_strip_refresh(led_strip);
    } else {
        /* Set all LED off to clear all pixels */
        led_strip_clear(led_strip);
    }
}

static void configure_led(uint32_t leds)
{
    ESP_LOGI(TAG, "Example configured to blink addressable LED!");
    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = 10,
        .max_leds = leds, // at least one LED on board
        .led_pixel_format = LED_PIXEL_FORMAT_GRBW,
        .led_model = LED_MODEL_SK6812,
    };

    // led_strip_config_t strip_config = {
    //     .strip_gpio_num = 3,
    //     .max_leds = leds
    // };

    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));

    /* Set all LED off to clear all pixels */
    led_strip_clear(led_strip);
}

void free_memory(void *pvParameter) {
    while (1) {
        uint32_t freeHeapBytes = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
        printf("Free mem:%ld\n", freeHeapBytes);

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    vTaskDelete( NULL ); 
}

void led_sequence_task(void *pvParameter) {
    gpio_reset_pin(GPIO_NUM_8);
    gpio_reset_pin(GPIO_NUM_7);
    gpio_reset_pin(GPIO_NUM_6);
    gpio_set_direction(GPIO_NUM_8, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_NUM_7, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_NUM_6, GPIO_MODE_OUTPUT);

    while (1) {
        gpio_set_level(GPIO_NUM_6, 0);
        gpio_set_level(GPIO_NUM_8, 1);     
        vTaskDelay(300 / portTICK_PERIOD_MS);
        gpio_set_level(GPIO_NUM_8, 0);
        gpio_set_level(GPIO_NUM_7, 1);
        vTaskDelay(300 / portTICK_PERIOD_MS);
        gpio_set_level(GPIO_NUM_7, 0);
        gpio_set_level(GPIO_NUM_6, 1);
        vTaskDelay(300 / portTICK_PERIOD_MS);
    }
    vTaskDelete( NULL );  
}

struct js *run_leds_rain_setup(double matrixHeight, double matrixWidth, double start, double time, double last, double number) { 
    struct js *jsEngine = js_create(jsBuf, sizeof(jsBuf));
    jsval_t global = js_glob(jsEngine);
    jsval_t matrix = js_mkobj(jsEngine);
    jsval_t frame = js_mkobj(jsEngine);

    js_set(jsEngine, global, "delay", js_mkfun(myDelay));
    js_set(jsEngine, global, "setPixelColor", js_mkfun(setPixelColor));
    js_set(jsEngine, global, "updatePixels", js_mkfun(updatePixels));
    js_set(jsEngine, global, "matrix", matrix);
    js_set(jsEngine, global, "frame", frame);
    
    js_set(jsEngine, matrix, "height", js_mknum(matrixHeight));
    js_set(jsEngine, matrix, "width", js_mknum(matrixWidth));

    js_set(jsEngine, frame, "start", js_mknum(start));
    js_set(jsEngine, frame, "time", js_mknum(time));
    js_set(jsEngine, frame, "last", js_mknum(last));
    js_set(jsEngine, frame, "number", js_mknum(number));

    return jsEngine;
}

void run_leds_rain(struct js *jsEngine) { 
    js_eval(jsEngine,
            js_content,
            ~0U);
}

void rainbow_sequence_task(void *pvParameter) {
    printf("Start JS demo\n");

    double matrixHeight = 55;
    double matrixWidth = 1;

    configure_led(matrixHeight);

    double startTime = (double)esp_timer_get_time();
    double lastTime = startTime;
    double time = startTime;
    double number = 0;
    
    

    double fps = 30;
    double n = startTime - lastTime;
    double t = startTime - time;
    double f = (1 / fps) * 1000;
    double fn = t / f;
    double tf = (t / 1000) * fps;
    double mf = fn - tf;
    //double number = 255 * (mf / fps);

    number++;

    while (1) {  
        lastTime = time;
        time = (double)esp_timer_get_time();
        
        struct js *jsEngine = run_leds_rain_setup(matrixHeight, matrixWidth, startTime, time, lastTime, number);  
        run_leds_rain(jsEngine); 
    }
    vTaskDelete( NULL );  
}

extern "C" void app_main()
{   
    xTaskCreate(
    &led_sequence_task,
    "led_sequence_task",
    2048,
    NULL,
    5,
    NULL);

    xTaskCreate(
    &free_memory,
    "free_memory",
    2048,
    NULL,
    1,
    NULL);

    xTaskCreate(
    &rainbow_sequence_task,
    "rainbow_sequence_task",
    16384,
    NULL,
    5,
    NULL);   
}