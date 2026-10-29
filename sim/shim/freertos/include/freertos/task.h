#pragma once

#include "FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

BaseType_t xTaskCreate(TaskFunction_t pxTaskCode,
                       const char * const pcName,
                       const configSTACK_DEPTH_TYPE usStackDepth,
                       void * const pvParameters,
                       UBaseType_t uxPriority,
                       TaskHandle_t * const pxCreatedTask);

BaseType_t xTaskCreatePinnedToCore(TaskFunction_t pxTaskCode,
                                   const char * const pcName,
                                   const configSTACK_DEPTH_TYPE usStackDepth,
                                   void * const pvParameters,
                                   UBaseType_t uxPriority,
                                   TaskHandle_t * const pxCreatedTask,
                                   const BaseType_t xCoreID);

BaseType_t xTaskCreatePinnedToCoreWithCaps(TaskFunction_t pxTaskCode,
                                           const char * const pcName,
                                           const uint32_t usStackDepth,
                                           void * const pvParameters,
                                           UBaseType_t uxPriority,
                                           TaskHandle_t * const pxCreatedTask,
                                           const BaseType_t xCoreID,
                                           const uint32_t uxCaps);

void vTaskDelete(TaskHandle_t xTaskToDelete);

void vTaskDelay(const TickType_t xTicksToDelay);

void vTaskDelayUntil(TickType_t * const pxPreviousWakeTime, const TickType_t xTimeIncrement);

TaskHandle_t xTaskGetCurrentTaskHandle(void);

const char * pcTaskGetName(TaskHandle_t xTaskToQuery);

UBaseType_t uxTaskPriorityGet(const TaskHandle_t xTask);

void vTaskPrioritySet(TaskHandle_t xTask, UBaseType_t uxNewPriority);

void vTaskNotifyGiveFromISR(TaskHandle_t xTaskToNotify, BaseType_t *pxHigherPriorityTaskWoken);

uint32_t ulTaskNotifyTake(BaseType_t xClearCountOnExit, TickType_t xTicksToWait);

BaseType_t xTaskNotifyGive(TaskHandle_t xTaskToNotify);

#ifdef __cplusplus
}
#endif
