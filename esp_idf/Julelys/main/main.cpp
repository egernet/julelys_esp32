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

#include <esp_heap_caps.h>

#include "js_string.h"

#include "led_controller.h"
#include "js_controller.h"
#include "settings_controller.h"

#include "driver/uart.h"
#include "driver/gpio.h"

#include "iot_button.h"
#include "button_gpio.h"

static const char TAG[] = "Jylelys";

#ifndef IP4ADDR_STRLEN_MAX
#define IP4ADDR_STRLEN_MAX  16
#endif

#define TXD_PIN (GPIO_NUM_17)
#define RXD_PIN (GPIO_NUM_16)
#define BUF_SIZE (1024)

LedController *ledController;
SettingsController *settingsController;

#define BUTTON_PIN GPIO_NUM_9
#define LONG_PRESS_DELAY 10000

static uint32_t button_press_time = 0;

void button_single_click_cb(void *arg,void *usr_data) {
    ESP_LOGI(TAG, "BUTTON_SINGLE_CLICK");
}

static void button_long_press_start_cb(void *arg,void *usr_data) {
    settingsController->reset();
    esp_restart();
}

void configure_button() {
    button_config_t gpio_btn_cfg = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = CONFIG_BUTTON_LONG_PRESS_TIME_MS,
        .short_press_time = CONFIG_BUTTON_SHORT_PRESS_TIME_MS,
        .gpio_button_config = {
            .gpio_num = 9,
            .active_level = 0,
        },
    };
    
    button_handle_t gpio_btn = iot_button_create(&gpio_btn_cfg);
    if(NULL == gpio_btn) {
        ESP_LOGE(TAG, "Button create failed");
        return;
    }

    iot_button_register_cb(gpio_btn, BUTTON_SINGLE_CLICK, button_single_click_cb, NULL);
    iot_button_register_cb(gpio_btn, BUTTON_LONG_PRESS_START, button_long_press_start_cb, NULL);
}

void free_memory(void *pvParameter) {
    while (1) {
        uint32_t freeHeapBytes = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
        printf("Free mem:%ld\n", freeHeapBytes);

        vTaskDelay(60000 / portTICK_PERIOD_MS);
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

void led_setting_sequence_task(void *pvParameter) {
    gpio_reset_pin(GPIO_NUM_8);
    gpio_reset_pin(GPIO_NUM_7);
    gpio_reset_pin(GPIO_NUM_6);
    gpio_set_direction(GPIO_NUM_8, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_NUM_7, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_NUM_6, GPIO_MODE_OUTPUT);

    gpio_set_level(GPIO_NUM_8, 0);
    gpio_set_level(GPIO_NUM_7, 0);
    gpio_set_level(GPIO_NUM_6, 0);

    while (1) {
        gpio_set_level(GPIO_NUM_8, 1);     
        vTaskDelay(300 / portTICK_PERIOD_MS);
        gpio_set_level(GPIO_NUM_8, 0);
        vTaskDelay(300 / portTICK_PERIOD_MS);
    }
    vTaskDelete( NULL );  
}

void js_sequence_task(void *pvParameter) {
    printf("Start JS demo\n");

    JSController *jsController = new JSController(ledController);

    jsController->runCode(js_content);

    delete jsController;

    vTaskDelete( NULL );  
}

void init_uart() {
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS,
        .rx_flow_ctrl_thresh = 122,
    };
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM_1, BUF_SIZE * 2, 0, 0, NULL, 0);
}

void uart_task(void *arg) {
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
    while (1) {
        int len = uart_read_bytes(UART_NUM_1, data, BUF_SIZE, 20 / portTICK_PERIOD_MS);
        if (len > 0) {
            uart_write_bytes(UART_NUM_1, (const char *) data, len);
        }
    }
    free(data);
}

extern "C" {
    void app_main();
}

void app_main()
{   
    settingsController = new SettingsController();

    int number = settingsController->loadConfig();

    if (number == 0 ) {
        printf("Vi venter på data");

        xTaskCreate(
        &led_setting_sequence_task,
        "led_setting_sequence_task",
        2048,
        NULL,
        5,
        NULL);

        settingsController->saveConfig(512);

        vTaskDelay(5000 / portTICK_PERIOD_MS);

        esp_restart();
    } else {
        configure_button();

        printf("Setting data: %d", number);
        ledController = new LedController(10, 1, 55);

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
        &js_sequence_task,
        "js_sequence_task",
        16384,
        NULL,
        5,
        NULL);
    }   
}