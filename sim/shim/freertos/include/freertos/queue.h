#pragma once

#include "FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

#define errQUEUE_EMPTY ((BaseType_t)0)
#define errQUEUE_FULL  ((BaseType_t)0)

QueueHandle_t xQueueCreate(UBaseType_t uxQueueLength, UBaseType_t uxItemSize);

BaseType_t xQueueSend(QueueHandle_t xQueue, const void * const pvItemToQueue, TickType_t xTicksToWait);
BaseType_t xQueueSendToBack(QueueHandle_t xQueue, const void * const pvItemToQueue, TickType_t xTicksToWait);
BaseType_t xQueueSendToFront(QueueHandle_t xQueue, const void * const pvItemToQueue, TickType_t xTicksToWait);

BaseType_t xQueueReceive(QueueHandle_t xQueue, void * const pvBuffer, TickType_t xTicksToWait);
BaseType_t xQueuePeek(QueueHandle_t xQueue, void * const pvBuffer, TickType_t xTicksToWait);

UBaseType_t uxQueueMessagesWaiting(const QueueHandle_t xQueue);
UBaseType_t uxQueueSpacesAvailable(const QueueHandle_t xQueue);

void vQueueDelete(QueueHandle_t xQueue);
void xQueueReset(QueueHandle_t xQueue);

#define xQueueSendFromISR(xQueue, pvItemToQueue, pxHigherPriorityTaskWoken) xQueueSend(xQueue, pvItemToQueue, 0)
#define xQueueReceiveFromISR(xQueue, pvBuffer, pxHigherPriorityTaskWoken) xQueueReceive(xQueue, pvBuffer, 0)

#ifdef __cplusplus
}
#endif
