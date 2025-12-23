#pragma once

// ESP logging mocks - just print to stdout
#include <cstdio>

#define ESP_LOGI(tag, fmt, ...) printf("[INFO] %s: " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGW(tag, fmt, ...) printf("[WARN] %s: " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGE(tag, fmt, ...) printf("[ERROR] %s: " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGD(tag, fmt, ...)

#define ESP_ERROR_CHECK(x) do { (void)(x); } while(0)
