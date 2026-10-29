#include "esp_timer.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

struct esp_timer {
    esp_timer_cb_t callback = nullptr;
    void* arg = nullptr;
    std::string name;
    std::thread worker;
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> is_running{false};
    std::atomic<bool> stop_flag{false};
    uint64_t period_us = 0;
    bool periodic = false;
};

extern "C" {

esp_err_t esp_timer_create(const esp_timer_create_args_t* create_args,
                           esp_timer_handle_t* out_handle) {
    if (create_args == nullptr || out_handle == nullptr || create_args->callback == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    auto* t = new esp_timer();
    t->callback = create_args->callback;
    t->arg = create_args->arg;
    if (create_args->name != nullptr) {
        t->name = create_args->name;
    }
    *out_handle = t;
    return ESP_OK;
}

static esp_err_t start_timer_internal(esp_timer_handle_t timer, uint64_t interval_us, bool periodic) {
    if (timer == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    if (timer->is_running) {
        esp_timer_stop(timer);
    }
    timer->period_us = interval_us;
    timer->periodic = periodic;
    timer->stop_flag = false;
    timer->is_running = true;

    timer->worker = std::thread([timer]() {
        while (!timer->stop_flag) {
            std::unique_lock<std::mutex> lock(timer->mtx);
            if (timer->cv.wait_for(lock, std::chrono::microseconds(timer->period_us),
                                   [timer]() { return timer->stop_flag.load(); })) {
                break;
            }
            if (timer->stop_flag) {
                break;
            }
            lock.unlock();
            if (timer->callback != nullptr) {
                timer->callback(timer->arg);
            }
            if (!timer->periodic) {
                break;
            }
        }
        timer->is_running = false;
    });

    return ESP_OK;
}

esp_err_t esp_timer_start_once(esp_timer_handle_t timer, uint64_t timeout_us) {
    return start_timer_internal(timer, timeout_us, false);
}

esp_err_t esp_timer_start_periodic(esp_timer_handle_t timer, uint64_t period_us) {
    return start_timer_internal(timer, period_us, true);
}

esp_err_t esp_timer_stop(esp_timer_handle_t timer) {
    if (timer == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    timer->stop_flag = true;
    timer->cv.notify_all();
    if (timer->worker.joinable()) {
        timer->worker.join();
    }
    timer->is_running = false;
    return ESP_OK;
}

esp_err_t esp_timer_delete(esp_timer_handle_t timer) {
    if (timer == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_timer_stop(timer);
    delete timer;
    return ESP_OK;
}

int64_t esp_timer_get_time(void) {
    using clock = std::chrono::steady_clock;
    return static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(clock::now().time_since_epoch()).count());
}

}  // extern "C"
