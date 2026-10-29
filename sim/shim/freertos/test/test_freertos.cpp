#include <cassert>
#include <cstdio>
#include <atomic>
#include <chrono>
#include <thread>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "esp_err.h"

static std::atomic<int> s_task_counter{0};
static std::atomic<int> s_timer_counter{0};

static void sample_task(void* arg) {
    int val = static_cast<int>(reinterpret_cast<intptr_t>(arg));
    s_task_counter += val;
    vTaskDelay(10);
    s_task_counter += 1;
    vTaskDelete(NULL);
}

static void timer_cb(void* arg) {
    (void)arg;
    s_timer_counter++;
}

int main() {
    printf("=== Running FreeRTOS & esp_timer Shim Unit Tests ===\n");

    // Test 1: Task creation & execution
    printf("[1] Testing xTaskCreate...\n");
    TaskHandle_t task = nullptr;
    BaseType_t res = xTaskCreate(sample_task, "test_task", 4096, (void*)(intptr_t)10, 1, &task);
    assert(res == pdPASS);
    // Wait for task to finish
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    assert(s_task_counter == 11);
    printf("    xTaskCreate & vTaskDelay passed!\n");

    // Test 2: Event Groups
    printf("[2] Testing Event Groups...\n");
    EventGroupHandle_t eg = xEventGroupCreate();
    assert(eg != nullptr);
    assert(xEventGroupGetBits(eg) == 0);

    xEventGroupSetBits(eg, 0x01 | 0x04);
    assert(xEventGroupGetBits(eg) == 0x05);

    EventBits_t waited = xEventGroupWaitBits(eg, 0x01, pdTRUE, pdFALSE, 100);
    assert((waited & 0x01) == 0x01);
    // After clear on exit, bit 0 should be 0, bit 2 should still be 1
    assert((xEventGroupGetBits(eg) & 0x01) == 0);
    assert((xEventGroupGetBits(eg) & 0x04) == 0x04);

    vEventGroupDelete(eg);
    printf("    Event Groups passed!\n");

    // Test 3: Queues
    printf("[3] Testing Queues...\n");
    QueueHandle_t q = xQueueCreate(5, sizeof(int));
    assert(q != nullptr);
    assert(uxQueueMessagesWaiting(q) == 0);

    for (int i = 1; i <= 3; ++i) {
        assert(xQueueSend(q, &i, 10) == pdPASS);
    }
    assert(uxQueueMessagesWaiting(q) == 3);

    for (int i = 1; i <= 3; ++i) {
        int val = 0;
        assert(xQueueReceive(q, &val, 10) == pdPASS);
        assert(val == i);
    }
    assert(uxQueueMessagesWaiting(q) == 0);
    vQueueDelete(q);
    printf("    Queues passed!\n");

    // Test 4: Semaphores & Mutexes
    printf("[4] Testing Semaphore / Mutex...\n");
    SemaphoreHandle_t sem = xSemaphoreCreateMutex();
    assert(sem != nullptr);
    assert(xSemaphoreTake(sem, 10) == pdTRUE);
    assert(xSemaphoreGive(sem) == pdTRUE);
    vSemaphoreDelete(sem);
    printf("    Semaphore / Mutex passed!\n");

    // Test 5: esp_timer
    printf("[5] Testing esp_timer...\n");
    int64_t t1 = esp_timer_get_time();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    int64_t t2 = esp_timer_get_time();
    assert(t2 > t1);

    esp_timer_create_args_t timer_args = {
        .callback = timer_cb,
        .arg = nullptr,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "test_timer",
        .skip_unhandled_events = false
    };
    esp_timer_handle_t thandle = nullptr;
    assert(esp_timer_create(&timer_args, &thandle) == ESP_OK);
    assert(thandle != nullptr);

    // Run periodic at 10ms (10,000 us)
    s_timer_counter = 0;
    assert(esp_timer_start_periodic(thandle, 10000) == ESP_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(45));
    assert(esp_timer_stop(thandle) == ESP_OK);
    int count = s_timer_counter.load();
    assert(count >= 3 && count <= 6);
    assert(esp_timer_delete(thandle) == ESP_OK);
    printf("    esp_timer passed (ticks=%d)!\n", count);

    // Test 6: esp_err_to_name
    printf("[6] Testing esp_err_to_name...\n");
    assert(strcmp(esp_err_to_name(ESP_OK), "ESP_OK") == 0);
    assert(strcmp(esp_err_to_name(ESP_FAIL), "ESP_FAIL") == 0);
    printf("    esp_err_to_name passed!\n");

    printf("=== All FreeRTOS & esp_timer Shim Tests PASSED! ===\n");
    return 0;
}
