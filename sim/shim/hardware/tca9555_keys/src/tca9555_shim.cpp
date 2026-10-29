#include "sim_hardware.h"
#include "epd_board_metalio_eink4_plus.h"
#include "config.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <mutex>
#include <thread>

namespace {

std::mutex s_hw_mtx;
std::condition_variable s_tca_cv;

// Port 0 outputs:
// P0.0 = VCOM, P0.1 = PWRUP, P0.2 = WAKEUP, P0.3 = MOTOR, P0.4 = PA_EN, P0.5 = CAM_SCR_EN
uint8_t s_port0 = 0x00;

// Port 1 inputs/outputs:
// P1.0 = VOL+ (in, idle 1), P1.1 = VOL- (in, idle 1), P1.2 = TP_RST (out, idle 1)
// P1.3 = PWR_PULSE (out, idle 0), P1.4 = BT_PA_PWR (out, idle 1)
// P1.6 = RTC_INT (in, idle 1), P1.7 = POWER (in, idle 1)
uint8_t s_port1 = 0xFF;

bool s_boot_down = false;
bool s_motor_on = false;
bool s_pa_on = false;
int s_shutdown_pulse_count = 0;
bool s_tca_irq_pending = false;
TaskHandle_t s_gpio2_subscribed_task = nullptr;

void NotifyTcaInterruptLocked() {
    s_tca_irq_pending = true;
    s_tca_cv.notify_all();
    if (s_gpio2_subscribed_task != nullptr) {
        BaseType_t hp = pdFALSE;
        vTaskNotifyGiveFromISR(s_gpio2_subscribed_task, &hp);
    }
}

} // namespace

namespace sim::hw {

void SetKey(Key key, bool pressed) {
    std::lock_guard<std::mutex> lock(s_hw_mtx);
    switch (key) {
        case Key::kBoot: {
            s_boot_down = pressed;
            // BOOT button: active low (pressed=0, released=1)
            sim_gpio_set_level(BOOT_BUTTON_GPIO, pressed ? 0 : 1);
            break;
        }
        case Key::kVolUp: {
            // VOL+ is P1.0 (bit 0). Pressed = 0, Released = 1
            if (pressed) {
                s_port1 &= ~(1u << 0);
            } else {
                s_port1 |= (1u << 0);
            }
            NotifyTcaInterruptLocked();
            break;
        }
        case Key::kVolDown: {
            // VOL- is P1.1 (bit 1). Pressed = 0, Released = 1
            if (pressed) {
                s_port1 &= ~(1u << 1);
            } else {
                s_port1 |= (1u << 1);
            }
            NotifyTcaInterruptLocked();
            break;
        }
        case Key::kPower: {
            // POWER is P1.7 (bit 7). Pressed = 0, Released = 1
            if (pressed) {
                s_port1 &= ~(1u << 7);
            } else {
                s_port1 |= (1u << 7);
            }
            NotifyTcaInterruptLocked();
            break;
        }
    }
}

void PressKey(Key key, uint32_t hold_ms) {
    SetKey(key, true);
    std::this_thread::sleep_for(std::chrono::milliseconds(hold_ms));
    SetKey(key, false);
}

bool IsKeyDown(Key key) {
    std::lock_guard<std::mutex> lock(s_hw_mtx);
    switch (key) {
        case Key::kBoot:
            return s_boot_down;
        case Key::kVolUp:
            return (s_port1 & (1u << 0)) == 0;
        case Key::kVolDown:
            return (s_port1 & (1u << 1)) == 0;
        case Key::kPower:
            return (s_port1 & (1u << 7)) == 0;
    }
    return false;
}

bool GetMotorState() {
    std::lock_guard<std::mutex> lock(s_hw_mtx);
    return s_motor_on;
}

bool GetPaState() {
    std::lock_guard<std::mutex> lock(s_hw_mtx);
    return s_pa_on;
}

int GetShutdownPulseCount() {
    std::lock_guard<std::mutex> lock(s_hw_mtx);
    return s_shutdown_pulse_count;
}

void ResetHardwareState() {
    std::lock_guard<std::mutex> lock(s_hw_mtx);
    s_port0 = 0x00;
    s_port1 = 0xFF;
    s_boot_down = false;
    s_motor_on = false;
    s_pa_on = false;
    s_shutdown_pulse_count = 0;
    s_tca_irq_pending = false;
    s_gpio2_subscribed_task = nullptr;
    sim_gpio_set_level(BOOT_BUTTON_GPIO, 1);
}

} // namespace sim::hw

// ===========================================================================
// C Hardware Abstraction Layer (epd_board_metalio_eink4_plus.h implementation)
// ===========================================================================

extern "C" {

bool epd_board_metalio_eink4_plus_tps_ok(void) {
    return true;
}

bool epd_board_metalio_eink4_plus_ioexp_ok(void) {
    return true;
}

void epd_board_metalio_eink4_plus_tp_reset(void) {
    std::lock_guard<std::mutex> lock(s_hw_mtx);
    // P1.2 TP_RST pulse low then high
    s_port1 &= ~(1u << 2);
    s_port1 |= (1u << 2);
}

esp_err_t epd_board_metalio_eink4_plus_camera_acquire(uint32_t /*timeout_ms*/) {
    return ESP_OK;
}

void epd_board_metalio_eink4_plus_camera_release(void) {
}

void epd_board_metalio_eink4_plus_motor_set(bool on) {
    std::lock_guard<std::mutex> lock(s_hw_mtx);
    s_motor_on = on;
    if (on) {
        s_port0 |= (1u << 3);
    } else {
        s_port0 &= ~(1u << 3);
    }
}

void epd_board_metalio_eink4_plus_pa_set(bool on) {
    std::lock_guard<std::mutex> lock(s_hw_mtx);
    s_pa_on = on;
    if (on) {
        s_port0 |= (1u << 4);
    } else {
        s_port0 &= ~(1u << 4);
    }
}

esp_err_t epd_board_metalio_eink4_plus_bt_power_reset(void) {
    return ESP_OK;
}

void epd_board_metalio_eink4_plus_ana_sw_set(bool /*high*/) {
}

void epd_board_metalio_eink4_plus_sleep_cut_rails(void) {
}

void epd_board_metalio_eink4_plus_sleep_restore_rails(void) {
}

bool epd_board_metalio_eink4_plus_ioexp_get_level(int io) {
    std::lock_guard<std::mutex> lock(s_hw_mtx);
    if (io < 8) {
        return (s_port0 & (1u << (io & 7))) != 0;
    } else {
        return (s_port1 & (1u << (io & 7))) != 0;
    }
}

bool epd_board_metalio_eink4_plus_ioexp_read_port(int port, uint8_t* out) {
    if (out == nullptr) return false;
    std::lock_guard<std::mutex> lock(s_hw_mtx);
    if (port == 0) {
        *out = s_port0;
        // Reading port 0 clears TCA INT on hardware
        s_tca_irq_pending = false;
    } else if (port == 1) {
        *out = s_port1;
    } else {
        return false;
    }
    return true;
}

bool epd_board_metalio_eink4_plus_ioexp_irq_wait(uint32_t timeout_ms) {
    std::unique_lock<std::mutex> lock(s_hw_mtx);
    if (s_tca_irq_pending) {
        s_tca_irq_pending = false;
        return true;
    }
    if (timeout_ms == 0) {
        return false;
    }
    s_tca_cv.wait_for(lock, std::chrono::milliseconds(timeout_ms), []() {
        return s_tca_irq_pending;
    });
    if (s_tca_irq_pending) {
        s_tca_irq_pending = false;
        return true;
    }
    return false;
}

void epd_board_metalio_eink4_plus_gpio2_irq_subscribe(TaskHandle_t task) {
    std::lock_guard<std::mutex> lock(s_hw_mtx);
    s_gpio2_subscribed_task = task;
}

void epd_board_metalio_eink4_plus_tps_enter_sleep(void) {
}

void epd_board_metalio_eink4_plus_bus_hold_low(void) {
}

void epd_board_metalio_eink4_plus_boot_release_holds(void) {
}

void epd_board_metalio_eink4_plus_bus_latch_for_sleep(void) {
}

void epd_board_metalio_eink4_plus_bus_restore(void) {
}

void epd_board_metalio_eink4_plus_ioexp_mask_for_sleep(void) {
}

bool epd_board_metalio_eink4_plus_ioexp_enable_power_wake_pin(void) {
    return true;
}

void epd_board_metalio_eink4_plus_ioexp_restore_keys(void) {
}

void epd_board_metalio_eink4_plus_pwr_key_pulse_train(void) {
    std::lock_guard<std::mutex> lock(s_hw_mtx);
    s_shutdown_pulse_count++;
    std::printf("[TCA9555 Shim] Shutdown pulse train dispatched on P13 (count=%d)\n", s_shutdown_pulse_count);
}

} // extern "C"
