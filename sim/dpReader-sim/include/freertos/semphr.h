#pragma once
#include "FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline SemaphoreHandle_t xSemaphoreCreateMutex(void) { return (SemaphoreHandle_t)1; }
static inline SemaphoreHandle_t xSemaphoreCreateBinary(void) { return (SemaphoreHandle_t)1; }
static inline BaseType_t xSemaphoreTake(SemaphoreHandle_t sem, TickType_t block_time) { (void)sem; (void)block_time; return pdTRUE; }
static inline BaseType_t xSemaphoreGive(SemaphoreHandle_t sem) { (void)sem; return pdTRUE; }
static inline void vSemaphoreDelete(SemaphoreHandle_t sem) { (void)sem; }

#ifdef __cplusplus
}
#endif
