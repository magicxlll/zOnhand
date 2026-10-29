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
typedef uint8_t StackType_t;
typedef struct { void* dummy; } StaticTask_t;
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

#ifndef portNUM_PROCESSORS
#define portNUM_PROCESSORS 2
#endif

typedef uint32_t configRUN_TIME_COUNTER_TYPE;

#define configSTACK_DEPTH_TYPE uint32_t
#define tskNO_AFFINITY ((BaseType_t)-1)

#ifndef BIT
#define BIT(nr) (1UL << (nr))
#endif
#ifndef BIT0
#define BIT0 (1UL << 0)
#define BIT1 (1UL << 1)
#define BIT2 (1UL << 2)
#define BIT3 (1UL << 3)
#define BIT4 (1UL << 4)
#define BIT5 (1UL << 5)
#define BIT6 (1UL << 6)
#define BIT7 (1UL << 7)
#define BIT8 (1UL << 8)
#define BIT9 (1UL << 9)
#define BIT10 (1UL << 10)
#define BIT11 (1UL << 11)
#define BIT12 (1UL << 12)
#define BIT13 (1UL << 13)
#define BIT14 (1UL << 14)
#define BIT15 (1UL << 15)
#define BIT16 (1UL << 16)
#define BIT17 (1UL << 17)
#define BIT18 (1UL << 18)
#define BIT19 (1UL << 19)
#define BIT20 (1UL << 20)
#define BIT21 (1UL << 21)
#define BIT22 (1UL << 22)
#define BIT23 (1UL << 23)
#define BIT24 (1UL << 24)
#define BIT25 (1UL << 25)
#define BIT26 (1UL << 26)
#define BIT27 (1UL << 27)
#define BIT28 (1UL << 28)
#define BIT29 (1UL << 29)
#define BIT30 (1UL << 30)
#define BIT31 (1UL << 31)
#endif

#ifndef IRAM_ATTR
#define IRAM_ATTR
#endif

#ifndef portYIELD_FROM_ISR
#define portYIELD_FROM_ISR() do {} while (0)
#endif

typedef void (*TaskFunction_t)(void *);

#ifdef __cplusplus
}
#endif
