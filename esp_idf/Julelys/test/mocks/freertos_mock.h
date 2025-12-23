#pragma once

#include <stdint.h>

// FreeRTOS type definitions
typedef void* SemaphoreHandle_t;
typedef void* TaskHandle_t;
typedef uint32_t TickType_t;
typedef int BaseType_t;

#define pdTRUE 1
#define pdFALSE 0
#define portMAX_DELAY 0xFFFFFFFF
#define portTICK_PERIOD_MS 1

// Semaphore mocks - simple pass-through
inline SemaphoreHandle_t xSemaphoreCreateMutex() { return (void*)1; }
inline BaseType_t xSemaphoreTake(SemaphoreHandle_t sem, TickType_t timeout) { (void)sem; (void)timeout; return pdTRUE; }
inline BaseType_t xSemaphoreGive(SemaphoreHandle_t sem) { (void)sem; return pdTRUE; }

// Task mocks
inline void vTaskDelay(TickType_t ticks) { (void)ticks; }
inline void vTaskDelete(TaskHandle_t task) { (void)task; }
inline BaseType_t xTaskCreate(void (*fn)(void*), const char* name, uint32_t stack, void* param, int prio, TaskHandle_t* handle) {
    (void)fn; (void)name; (void)stack; (void)param; (void)prio; (void)handle;
    return pdTRUE;
}

// Provide FreeRTOS header compatibility
#define freertos_FreeRTOS_h
#define freertos_semphr_h
#define freertos_task_h
