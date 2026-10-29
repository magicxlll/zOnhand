#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

enum { METALIO_AI_NONE = 0, METALIO_AI_SHORT = 1, METALIO_AI_LONG = 2 };

#define METALIO_FRONTLIGHT_RANGE 240

esp_err_t metalio_hw_init(void);
int metalio_take_volume(void);
void metalio_volume_clear(void);
void metalio_volume_sample(uint16_t inputs, int64_t now_ms);

int metalio_take_ai(void);
void metalio_ai_sample(bool down, int64_t now_ms);
void metalio_ai_clear(void);

bool metalio_confirm_power_key(void);
bool metalio_boot_power_key(void);
bool metalio_key_down(void);

esp_err_t metalio_io_read(uint16_t *inputs);
esp_err_t metalio_io_set(unsigned pin, bool high);

esp_err_t metalio_haptic_click(void);
void metalio_haptic_set_enabled(bool enabled);
bool metalio_haptic_enabled(void);
void metalio_haptic_stop(void);
void metalio_haptic_resume(void);

void metalio_frontlight(unsigned brightness, unsigned warmth);
unsigned metalio_brightness(void);
unsigned metalio_warmth(void);
bool metalio_frontlight_enabled(void);
unsigned metalio_frontlight_position(bool warmth);
void metalio_frontlight_preview_position(unsigned brightness, unsigned warmth);
void metalio_frontlight_restore(void);
void metalio_frontlight_save(unsigned brightness, unsigned warmth);
void metalio_frontlight_preview(unsigned brightness, unsigned warmth);
void metalio_frontlight_toggle(void);
void metalio_frontlight_commit(void);

esp_err_t metalio_tps_on(int vcom_mv);
esp_err_t metalio_tps_prepare(void);
void metalio_tps_cancel_prepare(void);
void metalio_tps_off(void);
bool metalio_tps_ready(void);
int metalio_tps_vcom_mv(void);

void metalio_clear_download_request(void);
esp_err_t metalio_usb_route_serial(bool serial);
void metalio_usb_serial_startup(void);
void metalio_usb_serial_reconnect(void);
bool metalio_usb_blocks_sleep(void);
bool metalio_download_connected(void);

esp_err_t metalio_sleep_sdk_init(void);
void metalio_hold_for_deep_sleep(void);
esp_err_t metalio_hold_for_light_sleep(void);
esp_err_t metalio_prepare_light_sleep_pins(void);
void metalio_release_light_sleep_pins(void);
esp_err_t metalio_sleep_cut_rails(uint16_t *saved);
esp_err_t metalio_sleep_restore_rails(uint16_t saved);
void metalio_sd_release_hold(void);
esp_err_t metalio_sd_single_line_idle(void);

// Simulator key injections
void sim_hw_inject_volume(int delta);
void sim_hw_inject_ai(int event);
void sim_hw_inject_power(void);

#ifdef __cplusplus
}
#endif
