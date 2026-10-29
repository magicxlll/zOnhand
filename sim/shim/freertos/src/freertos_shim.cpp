#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_err.h"

#include <chrono>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <memory>
#include <mutex>
#include <pthread.h>
#include <string>
#include <thread>
#include <vector>

namespace {

thread_local TaskHandle_t s_current_task = nullptr;

struct SimTask {
    std::string name;
    TaskFunction_t func = nullptr;
    void* param = nullptr;
    std::thread thread;
    std::atomic<bool> is_running{false};
    std::atomic<bool> detached{false};
    std::mutex notify_mtx;
    std::condition_variable notify_cv;
    uint32_t notify_val = 0;
};

struct SimEventGroup {
    std::mutex mtx;
    std::condition_variable cv;
    EventBits_t bits{0};
};

struct SimQueue {
    std::mutex mtx;
    std::condition_variable cv_not_full;
    std::condition_variable cv_not_empty;
    size_t item_size = 0;
    size_t max_items = 0;
    std::deque<std::vector<uint8_t>> items;
};

struct SimSemaphore {
    std::mutex mtx;
    std::condition_variable cv;
    bool is_mutex = false;
    UBaseType_t count = 0;
    UBaseType_t max_count = 1;
};

}  // namespace

extern "C" {

// ---------------------------------------------------------------------------
// FreeRTOS Task API
// ---------------------------------------------------------------------------

BaseType_t xTaskCreate(TaskFunction_t pxTaskCode,
                       const char * const pcName,
                       const configSTACK_DEPTH_TYPE /*usStackDepth*/,
                       void * const pvParameters,
                       UBaseType_t /*uxPriority*/,
                       TaskHandle_t * const pxCreatedTask) {
    if (pxTaskCode == nullptr) {
        return pdFAIL;
    }
    auto* task = new SimTask();
    task->name = pcName != nullptr ? pcName : "task";
    task->func = pxTaskCode;
    task->param = pvParameters;
    task->is_running = true;

    task->thread = std::thread([task]() {
        s_current_task = task;
        task->func(task->param);
        task->is_running = false;
    });

    if (pxCreatedTask != nullptr) {
        *pxCreatedTask = task;
    } else {
        task->detached = true;
        task->thread.detach();
    }
    return pdPASS;
}

BaseType_t xTaskCreatePinnedToCore(TaskFunction_t pxTaskCode,
                                   const char * const pcName,
                                   const configSTACK_DEPTH_TYPE usStackDepth,
                                   void * const pvParameters,
                                   UBaseType_t uxPriority,
                                   TaskHandle_t * const pxCreatedTask,
                                   const BaseType_t /*xCoreID*/) {
    return xTaskCreate(pxTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pxCreatedTask);
}

BaseType_t xTaskCreatePinnedToCoreWithCaps(TaskFunction_t pxTaskCode,
                                           const char * const pcName,
                                           const uint32_t usStackDepth,
                                           void * const pvParameters,
                                           UBaseType_t uxPriority,
                                           TaskHandle_t * const pxCreatedTask,
                                           const BaseType_t xCoreID,
                                           const uint32_t /*uxCaps*/) {
    return xTaskCreatePinnedToCore(pxTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pxCreatedTask, xCoreID);
}

void vTaskDelete(TaskHandle_t xTaskToDelete) {
    if (xTaskToDelete == nullptr || xTaskToDelete == s_current_task) {
        // Current task deleting itself
        SimTask* cur = static_cast<SimTask*>(s_current_task);
        if (cur != nullptr) {
            cur->is_running = false;
            if (!cur->detached) {
                cur->detached = true;
                cur->thread.detach();
            }
        }
        pthread_exit(nullptr);
        return;
    }
    SimTask* task = static_cast<SimTask*>(xTaskToDelete);
    if (task != nullptr) {
        task->is_running = false;
        if (task->thread.joinable()) {
            task->thread.detach();
        }
        delete task;
    }
}

void vTaskDelay(const TickType_t xTicksToDelay) {
    std::this_thread::sleep_for(std::chrono::milliseconds(xTicksToDelay * portTICK_PERIOD_MS));
}

void vTaskDelayUntil(TickType_t * const pxPreviousWakeTime, const TickType_t xTimeIncrement) {
    vTaskDelay(xTimeIncrement);
    if (pxPreviousWakeTime != nullptr) {
        *pxPreviousWakeTime += xTimeIncrement;
    }
}

TaskHandle_t xTaskGetCurrentTaskHandle(void) {
    return s_current_task;
}

const char * pcTaskGetName(TaskHandle_t xTaskToQuery) {
    SimTask* task = static_cast<SimTask*>(xTaskToQuery ? xTaskToQuery : s_current_task);
    return task != nullptr ? task->name.c_str() : "unknown";
}

UBaseType_t uxTaskPriorityGet(const TaskHandle_t /*xTask*/) {
    return 1;
}

void vTaskPrioritySet(TaskHandle_t /*xTask*/, UBaseType_t /*uxNewPriority*/) {
}

void vTaskNotifyGiveFromISR(TaskHandle_t xTaskToNotify, BaseType_t *pxHigherPriorityTaskWoken) {
    if (xTaskToNotify == nullptr) return;
    auto* task = static_cast<SimTask*>(xTaskToNotify);
    {
        std::lock_guard<std::mutex> lock(task->notify_mtx);
        task->notify_val++;
    }
    task->notify_cv.notify_one();
    if (pxHigherPriorityTaskWoken != nullptr) {
        *pxHigherPriorityTaskWoken = pdFALSE;
    }
}

BaseType_t xTaskNotifyGive(TaskHandle_t xTaskToNotify) {
    if (xTaskToNotify == nullptr) return pdFAIL;
    auto* task = static_cast<SimTask*>(xTaskToNotify);
    {
        std::lock_guard<std::mutex> lock(task->notify_mtx);
        task->notify_val++;
    }
    task->notify_cv.notify_one();
    return pdPASS;
}

uint32_t ulTaskNotifyTake(BaseType_t xClearCountOnExit, TickType_t xTicksToWait) {
    auto* cur = static_cast<SimTask*>(s_current_task);
    if (cur == nullptr) return 0;

    std::unique_lock<std::mutex> lock(cur->notify_mtx);
    if (cur->notify_val == 0 && xTicksToWait > 0) {
        if (xTicksToWait == portMAX_DELAY) {
            cur->notify_cv.wait(lock, [cur]() { return cur->notify_val > 0; });
        } else {
            cur->notify_cv.wait_for(lock, std::chrono::milliseconds(xTicksToWait * portTICK_PERIOD_MS),
                                    [cur]() { return cur->notify_val > 0; });
        }
    }

    uint32_t val = cur->notify_val;
    if (val > 0) {
        if (xClearCountOnExit == pdTRUE) {
            cur->notify_val = 0;
        } else {
            cur->notify_val--;
        }
    }
    return val;
}

// ---------------------------------------------------------------------------
// Event Groups API
// ---------------------------------------------------------------------------

EventGroupHandle_t xEventGroupCreate(void) {
    return new SimEventGroup();
}

EventBits_t xEventGroupSetBits(EventGroupHandle_t xEventGroup, const EventBits_t uxBitsToSet) {
    if (xEventGroup == nullptr) return 0;
    auto* grp = static_cast<SimEventGroup*>(xEventGroup);
    std::unique_lock<std::mutex> lock(grp->mtx);
    grp->bits |= uxBitsToSet;
    EventBits_t current = grp->bits;
    lock.unlock();
    grp->cv.notify_all();
    return current;
}

EventBits_t xEventGroupClearBits(EventGroupHandle_t xEventGroup, const EventBits_t uxBitsToClear) {
    if (xEventGroup == nullptr) return 0;
    auto* grp = static_cast<SimEventGroup*>(xEventGroup);
    std::lock_guard<std::mutex> lock(grp->mtx);
    EventBits_t prev = grp->bits;
    grp->bits &= ~uxBitsToClear;
    return prev;
}

EventBits_t xEventGroupGetBits(EventGroupHandle_t xEventGroup) {
    if (xEventGroup == nullptr) return 0;
    auto* grp = static_cast<SimEventGroup*>(xEventGroup);
    std::lock_guard<std::mutex> lock(grp->mtx);
    return grp->bits;
}

EventBits_t xEventGroupWaitBits(EventGroupHandle_t xEventGroup,
                                const EventBits_t uxBitsToWaitFor,
                                const BaseType_t xClearOnExit,
                                const BaseType_t xWaitForAllBits,
                                TickType_t xTicksToWait) {
    if (xEventGroup == nullptr) return 0;
    auto* grp = static_cast<SimEventGroup*>(xEventGroup);
    std::unique_lock<std::mutex> lock(grp->mtx);

    auto condition = [grp, uxBitsToWaitFor, xWaitForAllBits]() {
        if (xWaitForAllBits) {
            return (grp->bits & uxBitsToWaitFor) == uxBitsToWaitFor;
        } else {
            return (grp->bits & uxBitsToWaitFor) != 0;
        }
    };

    if (xTicksToWait == portMAX_DELAY) {
        grp->cv.wait(lock, condition);
    } else {
        grp->cv.wait_for(lock, std::chrono::milliseconds(xTicksToWait * portTICK_PERIOD_MS), condition);
    }

    EventBits_t ret = grp->bits;
    if (condition()) {
        if (xClearOnExit) {
            grp->bits &= ~uxBitsToWaitFor;
        }
    }
    return ret;
}

void vEventGroupDelete(EventGroupHandle_t xEventGroup) {
    if (xEventGroup != nullptr) {
        auto* grp = static_cast<SimEventGroup*>(xEventGroup);
        grp->cv.notify_all();
        delete grp;
    }
}

// ---------------------------------------------------------------------------
// Queue API
// ---------------------------------------------------------------------------

QueueHandle_t xQueueCreate(UBaseType_t uxQueueLength, UBaseType_t uxItemSize) {
    auto* q = new SimQueue();
    q->max_items = uxQueueLength > 0 ? uxQueueLength : 1;
    q->item_size = uxItemSize;
    return q;
}

BaseType_t xQueueSendToBack(QueueHandle_t xQueue, const void * const pvItemToQueue, TickType_t xTicksToWait) {
    if (xQueue == nullptr || pvItemToQueue == nullptr) return pdFAIL;
    auto* q = static_cast<SimQueue*>(xQueue);
    std::unique_lock<std::mutex> lock(q->mtx);

    auto not_full = [q]() { return q->items.size() < q->max_items; };
    if (!not_full()) {
        if (xTicksToWait == 0) return errQUEUE_FULL;
        if (xTicksToWait == portMAX_DELAY) {
            q->cv_not_full.wait(lock, not_full);
        } else {
            if (!q->cv_not_full.wait_for(lock, std::chrono::milliseconds(xTicksToWait * portTICK_PERIOD_MS), not_full)) {
                return errQUEUE_FULL;
            }
        }
    }

    std::vector<uint8_t> item(q->item_size);
    if (q->item_size > 0) {
        std::memcpy(item.data(), pvItemToQueue, q->item_size);
    }
    q->items.push_back(std::move(item));
    lock.unlock();
    q->cv_not_empty.notify_one();
    return pdPASS;
}

BaseType_t xQueueSend(QueueHandle_t xQueue, const void * const pvItemToQueue, TickType_t xTicksToWait) {
    return xQueueSendToBack(xQueue, pvItemToQueue, xTicksToWait);
}

BaseType_t xQueueSendToFront(QueueHandle_t xQueue, const void * const pvItemToQueue, TickType_t xTicksToWait) {
    if (xQueue == nullptr || pvItemToQueue == nullptr) return pdFAIL;
    auto* q = static_cast<SimQueue*>(xQueue);
    std::unique_lock<std::mutex> lock(q->mtx);

    auto not_full = [q]() { return q->items.size() < q->max_items; };
    if (!not_full()) {
        if (xTicksToWait == 0) return errQUEUE_FULL;
        if (xTicksToWait == portMAX_DELAY) {
            q->cv_not_full.wait(lock, not_full);
        } else {
            if (!q->cv_not_full.wait_for(lock, std::chrono::milliseconds(xTicksToWait * portTICK_PERIOD_MS), not_full)) {
                return errQUEUE_FULL;
            }
        }
    }

    std::vector<uint8_t> item(q->item_size);
    if (q->item_size > 0) {
        std::memcpy(item.data(), pvItemToQueue, q->item_size);
    }
    q->items.push_front(std::move(item));
    lock.unlock();
    q->cv_not_empty.notify_one();
    return pdPASS;
}

BaseType_t xQueueReceive(QueueHandle_t xQueue, void * const pvBuffer, TickType_t xTicksToWait) {
    if (xQueue == nullptr) return pdFAIL;
    auto* q = static_cast<SimQueue*>(xQueue);
    std::unique_lock<std::mutex> lock(q->mtx);

    auto not_empty = [q]() { return !q->items.empty(); };
    if (!not_empty()) {
        if (xTicksToWait == 0) return errQUEUE_EMPTY;
        if (xTicksToWait == portMAX_DELAY) {
            q->cv_not_empty.wait(lock, not_empty);
        } else {
            if (!q->cv_not_empty.wait_for(lock, std::chrono::milliseconds(xTicksToWait * portTICK_PERIOD_MS), not_empty)) {
                return errQUEUE_EMPTY;
            }
        }
    }

    if (pvBuffer != nullptr && q->item_size > 0) {
        std::memcpy(pvBuffer, q->items.front().data(), q->item_size);
    }
    q->items.pop_front();
    lock.unlock();
    q->cv_not_full.notify_one();
    return pdPASS;
}

BaseType_t xQueuePeek(QueueHandle_t xQueue, void * const pvBuffer, TickType_t xTicksToWait) {
    if (xQueue == nullptr) return pdFAIL;
    auto* q = static_cast<SimQueue*>(xQueue);
    std::unique_lock<std::mutex> lock(q->mtx);

    auto not_empty = [q]() { return !q->items.empty(); };
    if (!not_empty()) {
        if (xTicksToWait == 0) return errQUEUE_EMPTY;
        if (xTicksToWait == portMAX_DELAY) {
            q->cv_not_empty.wait(lock, not_empty);
        } else {
            if (!q->cv_not_empty.wait_for(lock, std::chrono::milliseconds(xTicksToWait * portTICK_PERIOD_MS), not_empty)) {
                return errQUEUE_EMPTY;
            }
        }
    }

    if (pvBuffer != nullptr && q->item_size > 0) {
        std::memcpy(pvBuffer, q->items.front().data(), q->item_size);
    }
    return pdPASS;
}

UBaseType_t uxQueueMessagesWaiting(const QueueHandle_t xQueue) {
    if (xQueue == nullptr) return 0;
    auto* q = static_cast<SimQueue*>(xQueue);
    std::lock_guard<std::mutex> lock(q->mtx);
    return q->items.size();
}

UBaseType_t uxQueueSpacesAvailable(const QueueHandle_t xQueue) {
    if (xQueue == nullptr) return 0;
    auto* q = static_cast<SimQueue*>(xQueue);
    std::lock_guard<std::mutex> lock(q->mtx);
    return q->max_items > q->items.size() ? (q->max_items - q->items.size()) : 0;
}

void vQueueDelete(QueueHandle_t xQueue) {
    if (xQueue != nullptr) {
        auto* q = static_cast<SimQueue*>(xQueue);
        delete q;
    }
}

void xQueueReset(QueueHandle_t xQueue) {
    if (xQueue != nullptr) {
        auto* q = static_cast<SimQueue*>(xQueue);
        std::lock_guard<std::mutex> lock(q->mtx);
        q->items.clear();
        q->cv_not_full.notify_all();
    }
}

// ---------------------------------------------------------------------------
// Semaphore API
// ---------------------------------------------------------------------------

SemaphoreHandle_t xSemaphoreCreateBinary(void) {
    auto* s = new SimSemaphore();
    s->is_mutex = false;
    s->count = 0;
    s->max_count = 1;
    return s;
}

SemaphoreHandle_t xSemaphoreCreateMutex(void) {
    auto* s = new SimSemaphore();
    s->is_mutex = true;
    s->count = 1;
    s->max_count = 1;
    return s;
}

SemaphoreHandle_t xSemaphoreCreateCounting(UBaseType_t uxMaxCount, UBaseType_t uxInitialCount) {
    auto* s = new SimSemaphore();
    s->is_mutex = false;
    s->count = uxInitialCount;
    s->max_count = uxMaxCount;
    return s;
}

BaseType_t xSemaphoreTake(SemaphoreHandle_t xSemaphore, TickType_t xTicksToWait) {
    if (xSemaphore == nullptr) return pdFAIL;
    auto* s = static_cast<SimSemaphore*>(xSemaphore);
    std::unique_lock<std::mutex> lock(s->mtx);

    auto available = [s]() { return s->count > 0; };
    if (!available()) {
        if (xTicksToWait == 0) return pdFALSE;
        if (xTicksToWait == portMAX_DELAY) {
            s->cv.wait(lock, available);
        } else {
            if (!s->cv.wait_for(lock, std::chrono::milliseconds(xTicksToWait * portTICK_PERIOD_MS), available)) {
                return pdFALSE;
            }
        }
    }

    s->count--;
    return pdTRUE;
}

BaseType_t xSemaphoreGive(SemaphoreHandle_t xSemaphore) {
    if (xSemaphore == nullptr) return pdFAIL;
    auto* s = static_cast<SimSemaphore*>(xSemaphore);
    std::unique_lock<std::mutex> lock(s->mtx);
    if (s->count < s->max_count) {
        s->count++;
        lock.unlock();
        s->cv.notify_one();
        return pdTRUE;
    }
    return pdFALSE;
}

void vSemaphoreDelete(SemaphoreHandle_t xSemaphore) {
    if (xSemaphore != nullptr) {
        auto* s = static_cast<SimSemaphore*>(xSemaphore);
        delete s;
    }
}

// ---------------------------------------------------------------------------
// ESP-IDF Error helper
// ---------------------------------------------------------------------------

const char *esp_err_to_name(esp_err_t code) {
    switch (code) {
        case ESP_OK: return "ESP_OK";
        case ESP_FAIL: return "ESP_FAIL";
        case ESP_ERR_NO_MEM: return "ESP_ERR_NO_MEM";
        case ESP_ERR_INVALID_ARG: return "ESP_ERR_INVALID_ARG";
        case ESP_ERR_INVALID_STATE: return "ESP_ERR_INVALID_STATE";
        case ESP_ERR_INVALID_SIZE: return "ESP_ERR_INVALID_SIZE";
        case ESP_ERR_NOT_FOUND: return "ESP_ERR_NOT_FOUND";
        case ESP_ERR_NOT_SUPPORTED: return "ESP_ERR_NOT_SUPPORTED";
        case ESP_ERR_TIMEOUT: return "ESP_ERR_TIMEOUT";
        default: return "ESP_ERR_UNKNOWN";
    }
}

void esp_restart(void) {
    std::fprintf(stderr, "[Sim] esp_restart requested - exiting.\n");
    std::exit(0);
}

}  // extern "C"
