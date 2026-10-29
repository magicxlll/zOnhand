#pragma once

#include "FreeRTOS.h"
#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

inline void vTaskDeleteWithCaps(TaskHandle_t task) {
    vTaskDelete(task);
}

#ifdef __cplusplus
}
#endif
