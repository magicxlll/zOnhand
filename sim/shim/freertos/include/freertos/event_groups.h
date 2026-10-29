#pragma once

#include "FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

EventGroupHandle_t xEventGroupCreate(void);

EventBits_t xEventGroupSetBits(EventGroupHandle_t xEventGroup, const EventBits_t uxBitsToSet);

EventBits_t xEventGroupClearBits(EventGroupHandle_t xEventGroup, const EventBits_t uxBitsToClear);

EventBits_t xEventGroupGetBits(EventGroupHandle_t xEventGroup);

EventBits_t xEventGroupWaitBits(EventGroupHandle_t xEventGroup,
                                const EventBits_t uxBitsToWaitFor,
                                const BaseType_t xClearOnExit,
                                const BaseType_t xWaitForAllBits,
                                TickType_t xTicksToWait);

void vEventGroupDelete(EventGroupHandle_t xEventGroup);

#ifdef __cplusplus
}
#endif
