#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t BaseType_t;
typedef uint32_t UBaseType_t;
typedef uint32_t TickType_t;
typedef void* TaskHandle_t;
typedef void* QueueHandle_t;
typedef QueueHandle_t SemaphoreHandle_t;
typedef void* EventGroupHandle_t;
typedef uint32_t EventBits_t;

#define pdFALSE ((BaseType_t)0)
#define pdTRUE  ((BaseType_t)1)
#define pdPASS  (pdTRUE)
#define pdFAIL  (pdFALSE)

#define portTICK_PERIOD_MS 1
#define pdMS_TO_TICKS(ms) ((TickType_t)(ms))
#define portMAX_DELAY ((TickType_t)0xFFFFFFFFUL)

#define configSTACK_DEPTH_TYPE uint32_t
#define tskNO_AFFINITY ((BaseType_t)-1)

typedef void (*TaskFunction_t)(void *);

#ifdef __cplusplus
}
#endif
