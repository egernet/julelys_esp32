#pragma once

#include <stdint.h>
#include <stddef.h>

// ESP-IDF type definitions
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL -1

// GPIO definitions
typedef int gpio_num_t;
#define GPIO_NUM_4 4
#define GPIO_NUM_5 5
#define GPIO_NUM_7 7
#define GPIO_NUM_8 8

typedef int gpio_mode_t;
#define GPIO_MODE_OUTPUT 1
