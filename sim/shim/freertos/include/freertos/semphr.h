#pragma once

#include "FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

SemaphoreHandle_t xSemaphoreCreateBinary(void);
SemaphoreHandle_t xSemaphoreCreateMutex(void);
SemaphoreHandle_t xSemaphoreCreateCounting(UBaseType_t uxMaxCount, UBaseType_t uxInitialCount);

BaseType_t xSemaphoreTake(SemaphoreHandle_t xSemaphore, TickType_t xTicksToWait);
BaseType_t xSemaphoreGive(SemaphoreHandle_t xSemaphore);

void vSemaphoreDelete(SemaphoreHandle_t xSemaphore);

#define xSemaphoreTakeFromISR(xSemaphore, pxHigherPriorityTaskWoken) xSemaphoreTake(xSemaphore, 0)
#define xSemaphoreGiveFromISR(xSemaphore, pxHigherPriorityTaskWoken) xSemaphoreGive(xSemaphore)

#ifdef __cplusplus
}
#endif
