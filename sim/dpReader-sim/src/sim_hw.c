#include "metalio_hw.h"
#include "read_pico_pmu.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

static int s_injected_vol = 0;
static int s_injected_ai = METALIO_AI_NONE;
static bool s_injected_power = false;

static unsigned s_brightness = 100;
static unsigned s_warmth = 50;
static bool s_haptic_enabled = true;

void sim_hw_inject_volume(int delta) {
    s_injected_vol = delta;
}

void sim_hw_inject_ai(int event) {
    s_injected_ai = event;
}

void sim_hw_inject_power(void) {
    s_injected_power = true;
}

esp_err_t metalio_hw_init(void) { return ESP_OK; }

int metalio_take_volume(void) {
    int v = s_injected_vol;
    s_injected_vol = 0;
    return v;
}

void metalio_volume_clear(void) {
    s_injected_vol = 0;
}

void metalio_volume_sample(uint16_t inputs, int64_t now_ms) {
    (void)inputs; (void)now_ms;
}

int metalio_take_ai(void) {
    int a = s_injected_ai;
    s_injected_ai = METALIO_AI_NONE;
    return a;
}

void metalio_ai_sample(bool down, int64_t now_ms) {
    (void)down; (void)now_ms;
}

void metalio_ai_clear(void) {
    s_injected_ai = METALIO_AI_NONE;
}

bool metalio_confirm_power_key(void) {
    bool p = s_injected_power;
    s_injected_power = false;
    return p;
}

read_pico_pmu_key_action_t read_pico_pmu_take_key_action(void) {
    if (s_injected_power) {
        s_injected_power = false;
        return READ_PICO_PMU_KEY_SHORT;
    }
    return READ_PICO_PMU_KEY_NONE;
}

bool read_pico_pmu_take_key_short(void) {
    return read_pico_pmu_take_key_action() == READ_PICO_PMU_KEY_SHORT;
}

bool metalio_boot_power_key(void) { return false; }
bool metalio_key_down(void) { return false; }

esp_err_t metalio_io_read(uint16_t *inputs) {
    if (inputs) *inputs = 0xFFFF;
    return ESP_OK;
}

esp_err_t metalio_io_set(unsigned pin, bool high) {
    (void)pin; (void)high;
    return ESP_OK;
}

esp_err_t metalio_haptic_click(void) { return ESP_OK; }
void metalio_haptic_set_enabled(bool enabled) { s_haptic_enabled = enabled; }
bool metalio_haptic_enabled(void) { return s_haptic_enabled; }
void metalio_haptic_stop(void) {}
void metalio_haptic_resume(void) {}

void metalio_frontlight(unsigned brightness, unsigned warmth) {
    s_brightness = brightness;
    s_warmth = warmth;
}
unsigned metalio_brightness(void) { return s_brightness; }
unsigned metalio_warmth(void) { return s_warmth; }
bool metalio_frontlight_enabled(void) { return s_brightness > 0; }
unsigned metalio_frontlight_position(bool warmth) { return warmth ? s_warmth : s_brightness; }
void metalio_frontlight_preview_position(unsigned brightness, unsigned warmth) { (void)brightness; (void)warmth; }
void metalio_frontlight_restore(void) {}
void metalio_frontlight_save(unsigned brightness, unsigned warmth) { s_brightness = brightness; s_warmth = warmth; }
void metalio_frontlight_preview(unsigned brightness, unsigned warmth) { (void)brightness; (void)warmth; }
void metalio_frontlight_toggle(void) {}
void metalio_frontlight_commit(void) {}

esp_err_t metalio_tps_on(int vcom_mv) { (void)vcom_mv; return ESP_OK; }
esp_err_t metalio_tps_prepare(void) { return ESP_OK; }
void metalio_tps_cancel_prepare(void) {}
void metalio_tps_off(void) {}
bool metalio_tps_ready(void) { return true; }
int metalio_tps_vcom_mv(void) { return 1700; }

void metalio_clear_download_request(void) {}
esp_err_t metalio_usb_route_serial(bool serial) { (void)serial; return ESP_OK; }
void metalio_usb_serial_startup(void) {}
void metalio_usb_serial_reconnect(void) {}
bool metalio_usb_blocks_sleep(void) { return false; }
bool metalio_download_connected(void) { return false; }

esp_err_t metalio_sleep_sdk_init(void) { return ESP_OK; }
void metalio_hold_for_deep_sleep(void) {}
esp_err_t metalio_hold_for_light_sleep(void) { return ESP_OK; }
esp_err_t metalio_prepare_light_sleep_pins(void) { return ESP_OK; }
void metalio_release_light_sleep_pins(void) {}
esp_err_t metalio_sleep_cut_rails(uint16_t *saved) { (void)saved; return ESP_OK; }
esp_err_t metalio_sleep_restore_rails(uint16_t saved) { (void)saved; return ESP_OK; }
void metalio_sd_release_hold(void) {}
esp_err_t metalio_sd_single_line_idle(void) { return ESP_OK; }
