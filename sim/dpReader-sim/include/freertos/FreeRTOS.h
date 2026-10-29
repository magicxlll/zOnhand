#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define pdMS_TO_TICKS(ms) (ms)
#define portMAX_DELAY 0xFFFFFFFFUL
#define pdTRUE 1
#define pdFALSE 0
#define pdPASS 1
#define pdFAIL 0

typedef void* TaskHandle_t;
typedef void* QueueHandle_t;
typedef void* SemaphoreHandle_t;
typedef uint32_t TickType_t;
typedef int BaseType_t;

typedef struct {
    int count;
} portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED {0}
#define portENTER_CRITICAL(mux) do { (void)(mux); } while(0)
#define portEXIT_CRITICAL(mux) do { (void)(mux); } while(0)
#define portENTER_CRITICAL_SAFE(mux) do { (void)(mux); } while(0)
#define portEXIT_CRITICAL_SAFE(mux) do { (void)(mux); } while(0)
#define portENTER_CRITICAL_ISR(mux) do { (void)(mux); } while(0)
#define portEXIT_CRITICAL_ISR(mux) do { (void)(mux); } while(0)

void vTaskDelay(uint32_t ticks);
static inline uint32_t uxTaskGetStackHighWaterMark(TaskHandle_t task) {
    (void)task;
    return 4096;
}

static inline uint32_t ulTaskNotifyTake(BaseType_t clear_count, TickType_t wait_ticks) {
    (void)clear_count; (void)wait_ticks;
    return 1;
}

static inline BaseType_t xTaskNotifyGive(TaskHandle_t task) {
    (void)task;
    return pdTRUE;
}

static inline void vTaskDelete(TaskHandle_t task) {
    if (task == NULL) {
        pthread_exit(NULL);
    }
}

struct sim_thread_arg_t {
    void (*task_func)(void*);
    void *params;
};

static inline void* sim_thread_trampoline(void* arg) {
    struct sim_thread_arg_t *ctx = (struct sim_thread_arg_t*)arg;
    void (*task_func)(void*) = ctx->task_func;
    void *params = ctx->params;
    free(ctx);
    task_func(params);
    return NULL;
}

static inline BaseType_t xTaskCreate(void (*task_func)(void*), const char* name, uint32_t stack_depth, void* params, unsigned priority, TaskHandle_t* out_task) {
    (void)name; (void)stack_depth; (void)priority;
    struct sim_thread_arg_t *ctx = (struct sim_thread_arg_t*)malloc(sizeof(struct sim_thread_arg_t));
    if (!ctx) return pdFAIL;
    ctx->task_func = task_func;
    ctx->params = params;
    pthread_t th;
    if (pthread_create(&th, NULL, sim_thread_trampoline, ctx) == 0) {
        pthread_detach(th);
        if (out_task) *out_task = (TaskHandle_t)(uintptr_t)th;
        return pdPASS;
    }
    free(ctx);
    return pdFAIL;
}

static inline BaseType_t xTaskCreatePinnedToCore(void (*task_func)(void*), const char* name, uint32_t stack_depth, void* params, unsigned priority, TaskHandle_t* out_task, int core_id) {
    (void)core_id;
    return xTaskCreate(task_func, name, stack_depth, params, priority, out_task);
}

#ifdef __cplusplus
}
#endif
