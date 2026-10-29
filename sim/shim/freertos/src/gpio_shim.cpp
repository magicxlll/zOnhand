#include "driver/gpio.h"
#include <mutex>
#include <unordered_map>
#include <cstdio>

namespace {

struct GpioPinState {
    int level = 0;
    gpio_mode_t mode = GPIO_MODE_INPUT;
    gpio_pullup_t pull_up = GPIO_PULLUP_DISABLE;
    gpio_pulldown_t pull_down = GPIO_PULLDOWN_DISABLE;
    gpio_int_type_t intr_type = GPIO_INTR_DISABLE;
    gpio_isr_t isr_handler = nullptr;
    void* isr_arg = nullptr;
};

std::mutex s_gpio_mtx;
std::unordered_map<int, GpioPinState> s_gpio_map;

GpioPinState& GetPin(int pin) {
    auto it = s_gpio_map.find(pin);
    if (it == s_gpio_map.end()) {
        GpioPinState state;
        // Default BOOT_BUTTON_GPIO (61) to idle high (pull-up)
        if (pin == 61) {
            state.level = 1;
            state.pull_up = GPIO_PULLUP_ENABLE;
        }
        s_gpio_map[pin] = state;
        return s_gpio_map[pin];
    }
    return it->second;
}

} // namespace

extern "C" {

esp_err_t gpio_config(const gpio_config_t *pGPIOConfig) {
    if (pGPIOConfig == nullptr) return ESP_ERR_INVALID_ARG;
    std::lock_guard<std::mutex> lock(s_gpio_mtx);
    for (int i = 0; i < 64; ++i) {
        if ((pGPIOConfig->pin_bit_mask & (1ULL << i)) != 0) {
            auto& pin = GetPin(i);
            pin.mode = pGPIOConfig->mode;
            pin.pull_up = pGPIOConfig->pull_up_en;
            pin.pull_down = pGPIOConfig->pull_down_en;
            pin.intr_type = pGPIOConfig->intr_type;
            if (pin.pull_up == GPIO_PULLUP_ENABLE) {
                pin.level = 1;
            } else if (pin.pull_down == GPIO_PULLDOWN_ENABLE) {
                pin.level = 0;
            }
        }
    }
    return ESP_OK;
}

esp_err_t gpio_reset_pin(gpio_num_t gpio_num) {
    std::lock_guard<std::mutex> lock(s_gpio_mtx);
    auto& pin = GetPin(static_cast<int>(gpio_num));
    pin.mode = GPIO_MODE_INPUT;
    pin.intr_type = GPIO_INTR_DISABLE;
    pin.isr_handler = nullptr;
    pin.isr_arg = nullptr;
    if (gpio_num == GPIO_NUM_61) {
        pin.level = 1;
        pin.pull_up = GPIO_PULLUP_ENABLE;
    } else {
        pin.level = 0;
        pin.pull_up = GPIO_PULLUP_DISABLE;
    }
    return ESP_OK;
}

esp_err_t gpio_set_level(gpio_num_t gpio_num, uint32_t level) {
    return sim_gpio_set_level(gpio_num, level);
}

int gpio_get_level(gpio_num_t gpio_num) {
    std::lock_guard<std::mutex> lock(s_gpio_mtx);
    return GetPin(static_cast<int>(gpio_num)).level;
}

esp_err_t gpio_set_direction(gpio_num_t gpio_num, gpio_mode_t mode) {
    std::lock_guard<std::mutex> lock(s_gpio_mtx);
    GetPin(static_cast<int>(gpio_num)).mode = mode;
    return ESP_OK;
}

esp_err_t gpio_set_pull_mode(gpio_num_t gpio_num, uint32_t pull) {
    std::lock_guard<std::mutex> lock(s_gpio_mtx);
    auto& pin = GetPin(static_cast<int>(gpio_num));
    if (pull == 0) {
        pin.pull_up = GPIO_PULLUP_ENABLE;
    } else if (pull == 1) {
        pin.pull_down = GPIO_PULLDOWN_ENABLE;
    }
    return ESP_OK;
}

esp_err_t gpio_install_isr_service(int /*no_use*/) {
    return ESP_OK;
}

esp_err_t gpio_isr_handler_add(gpio_num_t gpio_num, gpio_isr_t isr_handler, void *args) {
    std::lock_guard<std::mutex> lock(s_gpio_mtx);
    auto& pin = GetPin(static_cast<int>(gpio_num));
    pin.isr_handler = isr_handler;
    pin.isr_arg = args;
    return ESP_OK;
}

esp_err_t gpio_isr_handler_remove(gpio_num_t gpio_num) {
    std::lock_guard<std::mutex> lock(s_gpio_mtx);
    auto& pin = GetPin(static_cast<int>(gpio_num));
    pin.isr_handler = nullptr;
    pin.isr_arg = nullptr;
    return ESP_OK;
}

esp_err_t sim_gpio_set_level(gpio_num_t gpio_num, uint32_t level) {
    gpio_isr_t isr = nullptr;
    void* arg = nullptr;
    int old_level = 0;
    int new_level = level ? 1 : 0;
    gpio_int_type_t intr = GPIO_INTR_DISABLE;

    {
        std::lock_guard<std::mutex> lock(s_gpio_mtx);
        auto& pin = GetPin(static_cast<int>(gpio_num));
        old_level = pin.level;
        pin.level = new_level;
        isr = pin.isr_handler;
        arg = pin.isr_arg;
        intr = pin.intr_type;
    }

    if (old_level != new_level && isr != nullptr) {
        bool trigger = false;
        if (intr == GPIO_INTR_ANYEDGE) {
            trigger = true;
        } else if (intr == GPIO_INTR_NEGEDGE && old_level == 1 && new_level == 0) {
            trigger = true;
        } else if (intr == GPIO_INTR_POSEDGE && old_level == 0 && new_level == 1) {
            trigger = true;
        }
        if (trigger) {
            isr(arg);
        }
    }
    return ESP_OK;
}

} // extern "C"
