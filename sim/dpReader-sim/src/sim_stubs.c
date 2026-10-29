#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <time.h>
#include "esp_err.h"
#include "read_pico_pmu.h"
#include "read_pico_board.h"
#include "read_pico_transfer.h"
#include "esp_app_desc.h"
#include "epdiy.h"

static esp_app_desc_t s_app_desc = {
    .version = "v1.1.6-vn",
    .project_name = "dpReader",
    .time = "20:00:00",
    .date = "Oct 29 2026",
    .idf_ver = "v6.1.0"
};

const esp_app_desc_t *esp_app_get_description(void) {
    return &s_app_desc;
}

// PMU simulation
static pmu_snapshot_t s_pmu = {
    .present = true,
    .status_ok = true,
    .time_ok = true,
    .time_synced = true,
    .battery_mv = 4120,
    .soc_permille = 950, // 95.0%
    .charge_state = 0,   // discharging
    .unix_sec = 1730000000,
    .config = {
        .low_mv = 3300,
        .full_mv = 4200,
    }
};

bool read_pico_pmu_ready(void) { return true; }
const pmu_snapshot_t *read_pico_pmu_get(void) {
    s_pmu.unix_sec = (uint32_t)time(NULL);
    return &s_pmu;
}
esp_err_t read_pico_pmu_poll(void) { return ESP_OK; }
esp_err_t read_pico_pmu_refresh(void) { return ESP_OK; }
esp_err_t read_pico_pmu_cmd(uint16_t code, const uint8_t *payload, uint8_t plen) {
    (void)code; (void)payload; (void)plen;
    return ESP_OK;
}
esp_err_t read_pico_pmu_vcom_get(int* out_mv) {
    if (out_mv) *out_mv = 1700;
    return ESP_OK;
}
esp_err_t read_pico_pmu_vcom_set(int mv) { (void)mv; return ESP_OK; }
esp_err_t read_pico_pmu_uid_get(uint8_t out[10]) {
    if (out) memset(out, 0x11, 10);
    return ESP_OK;
}
void read_pico_pmu_drain_events(void) {}

// Board stubs
void read_pico_i2c_census_take(void) {}
bool read_pico_epd_release_drive(void) { return true; }
bool read_pico_sd_present(void) { return true; }
void read_pico_epd_restore_lcd(void) {}

// Transfer stubs
void read_pico_transfer_service_poll(void) {}
void read_pico_transfer_get_status(read_pico_transfer_status_t *out) {
    if (out) {
        memset(out, 0, sizeof(*out));
        out->state = READ_PICO_TRANSFER_STOPPED;
        out->mode = READ_PICO_TRANSFER_MODE_AP;
    }
}
esp_err_t read_pico_transfer_start(const read_pico_transfer_cfg_t *cfg) { (void)cfg; return ESP_OK; }
void read_pico_transfer_stop(void) {}
bool read_pico_transfer_radio_busy(void) { return false; }
esp_err_t read_pico_transfer_get_saved_wifi(char ssid[33], bool *configured) {
    if (ssid) ssid[0] = 0;
    if (configured) *configured = false;
    return ESP_OK;
}
bool read_pico_transfer_try_stop_if_idle(void) { return true; }
esp_err_t read_pico_transfer_preempt_for_network(void) { return ESP_OK; }
bool read_pico_transfer_pause_for_sleep(void) { return true; }
void read_pico_transfer_resume_after_sleep(void) {}

// Time service
void app_time_service_poll(bool idle) { (void)idle; }

// Sleep / Lock
esp_err_t app_enter_host_sleep(int mode) { (void)mode; return ESP_OK; }
void app_restart_host(void) {}
void app_lock_feedback(EpdiyHighlevelState *hl) { (void)hl; }
void app_lock_wait_key_idle(int ticks) { (void)ticks; }
void enter_lock_and_sleep(EpdiyHighlevelState *hl, int64_t *last_input_ms, void *touch, void *accel, bool draw_lock) {
    (void)hl; (void)last_input_ms; (void)touch; (void)accel; (void)draw_lock;
}
void app_lock_boot_gate(EpdiyHighlevelState *hl, void *touch, void *sensor) {
    (void)hl; (void)touch; (void)sensor;
}
void app_lock_capture_boot_wakeup(void) {}
bool app_lock_resume_frame(EpdiyHighlevelState *hl) { (void)hl; return false; }

// Lock PIN stubs
bool lock_pin_enabled(void) { return false; }
bool lock_pin_available(void) { return true; }
bool lock_pin_verify(const char pin[5]) { (void)pin; return true; }
esp_err_t lock_pin_replace(const char old_pin[5], const char new_pin[5]) { (void)old_pin; (void)new_pin; return ESP_OK; }
void lock_pin_wipe(void *bytes, unsigned length) { if (bytes && length) memset(bytes, 0, length); }

// BLE & OTA
bool ble_pt_connected(void) { return false; }
void pico_ota_confirm_running(void) {}
int vcom_setup_load_mv(void) { return 1700; }
void pmu_selftest_bind(void *sensor) { (void)sensor; }
void debug_panel_run(EpdiyHighlevelState *hl, uint8_t *fb, void *touch) { (void)hl; (void)fb; (void)touch; }
void log_export_init(void) {}
void rp_boot_mark(const char *tag) { (void)tag; }

// Crash and Log export
void crash_note_log(const char *msg) { (void)msg; }
void rp_crash_log_clear(void) {}
void log_export_clear(void) {}
const char* log_export_get_recent(void) { return ""; }
esp_err_t log_export_to_sd(void) { return ESP_OK; }

// App lifecycle & download
void app_cancel_download(void) {}
void app_enter_download(void) {}
void app_factory_reset(void) {}
void app_light_sleep_wait_timed(int ms) { (void)ms; }
void app_lock_set_resume_ready(bool ready) { (void)ready; }
bool app_sleep_media_ready(void) { return true; }
void app_sleep_discard_media_io(void) {}
bool lock_pin_grace_active(void) { return false; }
void online_font_cancel_join(void) {}

#include "online_font.h"

static const online_font_item_t s_font_catalog[ONLINE_FONT_COUNT] = {
    { .name = "Source Han Sans", .ascii_name = "SourceHanSans", .filename = "SourceHanSans.ttf" },
    { .name = "Source Han Serif", .ascii_name = "SourceHanSerif", .filename = "SourceHanSerif.ttf" },
    { .name = "Literata", .ascii_name = "Literata", .filename = "Literata.ttf" },
    { .name = "Charis SIL", .ascii_name = "CharisSIL", .filename = "CharisSIL.ttf" },
    { .name = "Bitter", .ascii_name = "Bitter", .filename = "Bitter.ttf" },
};

const online_font_item_t *online_font_item(unsigned index) {
    if (index >= ONLINE_FONT_COUNT) return NULL;
    return &s_font_catalog[index];
}

bool online_font_installed(unsigned index) {
    (void)index;
    return false;
}

esp_err_t online_font_start(unsigned index) {
    (void)index;
    return ESP_ERR_NOT_SUPPORTED;
}

void online_font_status(online_font_status_t *out) {
    if (out) {
        memset(out, 0, sizeof(*out));
        out->phase = ONLINE_FONT_IDLE;
    }
}

// Continuous DU
void continuous_du_init(void) {}
void continuous_du_reset(void) {}

// Transfer app & requests
#include "app.h"
#include "app_transfer_mode.h"

void app_transfer_request_method_picker(void) {}
void app_transfer_request_usb_start(void) {}
void app_transfer_request_wifi_setup(void) {}
void app_transfer_request_wifi_upload(void) {}
void app_transfer_request_hotspot_start(void) {}

static void transfer_stub_enter(app_ctx_t *ctx) { (void)ctx; }
static void transfer_stub_exit(app_ctx_t *ctx) { (void)ctx; }
static void transfer_stub_render(app_ctx_t *ctx, uint8_t *fb) { (void)ctx; (void)fb; }
static app_redraw_t transfer_stub_on_key(app_ctx_t *ctx, int key) { (void)ctx; (void)key; return APP_REDRAW_NONE; }
static app_redraw_t transfer_stub_on_gesture(app_ctx_t *ctx, const ui_gesture_event_t *event) { (void)ctx; (void)event; return APP_REDRAW_NONE; }
static app_redraw_t transfer_stub_on_tick(app_ctx_t *ctx) { (void)ctx; return APP_REDRAW_NONE; }

const app_desc_t app_transfer = {
    .title = "Transfer",
    .detail = "WiFi / USB",
    .on_enter = transfer_stub_enter,
    .on_exit = transfer_stub_exit,
    .render = transfer_stub_render,
    .on_key = transfer_stub_on_key,
    .on_gesture = transfer_stub_on_gesture,
    .on_tick = transfer_stub_on_tick,
};

// Kosync
char* kosync_md5_string(const char* str) { (void)str; return strdup(""); }
esp_err_t kosync_pull_book_progress(void* client, void* book, void* progress) { (void)client; (void)book; (void)progress; return ESP_ERR_NOT_SUPPORTED; }
esp_err_t kosync_push_book_progress(void* client, void* book, void* progress) { (void)client; (void)book; (void)progress; return ESP_ERR_NOT_SUPPORTED; }
esp_err_t kosync_sync_book(void* client, void* book) { (void)client; (void)book; return ESP_ERR_NOT_SUPPORTED; }
esp_err_t kosync_verify_account(void* client, const char* user, const char* pass, bool* out_ok) {
    (void)client; (void)user; (void)pass;
    if (out_ok) *out_ok = false;
    return ESP_ERR_NOT_SUPPORTED;
}

// OPDS
esp_err_t opds_download_book(void* client, const char* url, const char* path) { (void)client; (void)url; (void)path; return ESP_ERR_NOT_SUPPORTED; }
esp_err_t opds_fetch_feed(void* client, const char* url, void* feed) { (void)client; (void)url; (void)feed; return ESP_ERR_NOT_SUPPORTED; }

// Pico online & OTA
void pico_online_cancel_join(void) {}
esp_err_t pico_online_check(void* res) { (void)res; return ESP_ERR_NOT_SUPPORTED; }
esp_err_t pico_online_commit(void) { return ESP_OK; }
esp_err_t pico_online_download(void* req) { (void)req; return ESP_ERR_NOT_SUPPORTED; }
void pico_online_get_status(void* status) { (void)status; }
esp_err_t pico_ota_inspect(const char* path, void* info) { (void)path; (void)info; return ESP_ERR_NOT_SUPPORTED; }
esp_err_t pico_ota_install(const char* path) { (void)path; return ESP_ERR_NOT_SUPPORTED; }

// Transfer WiFi backup
esp_err_t read_pico_transfer_export_wifi_backup(read_pico_transfer_wifi_backup_t *backup) {
    if (backup) memset(backup, 0, sizeof(*backup));
    return ESP_OK;
}
esp_err_t read_pico_transfer_import_wifi_backup(const read_pico_transfer_wifi_backup_t *backup) {
    (void)backup;
    return ESP_OK;
}
bool read_pico_transfer_wifi_backup_valid(const read_pico_transfer_wifi_backup_t *backup) {
    (void)backup;
    return false;
}
esp_err_t read_pico_transfer_sync_time(uint32_t *utc_seconds) {
    if (utc_seconds) *utc_seconds = (uint32_t)time(NULL);
    return ESP_OK;
}

// BLE Page Turner
#include "ble_page_turner.h"

esp_err_t ble_pt_start(void) { return ESP_OK; }
void ble_pt_sync(bool wanted) { (void)wanted; }
bool ble_pt_take_start_failure(char *out, size_t cap) { (void)out; (void)cap; return false; }
void ble_pt_reset_failure(void) {}
esp_err_t ble_pt_stop(uint32_t timeout_ms) { (void)timeout_ms; return ESP_OK; }
esp_err_t ble_pt_suspend_for_sleep(void) { return ESP_OK; }
void ble_pt_resume_after_sleep(void) {}
bool ble_pt_running(void) { return false; }
esp_err_t ble_pt_network_acquire(void) { return ESP_OK; }
void ble_pt_network_release(void) {}
void ble_pt_poll(void) {}
uint32_t ble_pt_input_serial(void) { return 0; }
void ble_pt_scan_start(uint32_t ms) { (void)ms; }
void ble_pt_scan_stop(void) {}
bool ble_pt_scanning(void) { return false; }
uint8_t ble_pt_device_count(void) { return 0; }
const ble_pt_device_t *ble_pt_device(uint8_t index) { (void)index; return NULL; }
esp_err_t ble_pt_connect(const char *addr) { (void)addr; return ESP_OK; }
void ble_pt_reconnect_recent(void) {}
void ble_pt_cancel_recent(void) {}
void ble_pt_disconnect(void) {}
bool ble_pt_connecting(void) { return false; }
const char *ble_pt_connected_name(void) { return ""; }
bool ble_pt_take_failure(char *out, size_t cap) { (void)out; (void)cap; return false; }
void ble_pt_crumb(uint8_t id) { (void)id; }
bool ble_pt_take_passkey(uint32_t *out) { (void)out; return false; }
uint8_t ble_pt_bond_count(void) { return 0; }
const ble_pt_bond_t *ble_pt_bond(uint8_t index) { (void)index; return NULL; }
esp_err_t ble_pt_forget(const char *addr) { (void)addr; return ESP_OK; }
bool ble_pt_pop_key(ble_pt_event_t *out) { (void)out; return false; }
bool ble_pt_pop_raw(ble_pt_raw_t *out) { (void)out; return false; }
void ble_pt_learn_begin(void) {}
esp_err_t ble_pt_bind(ble_pt_action_t action, const ble_pt_frame_t *frame) { (void)action; (void)frame; return ESP_OK; }
bool ble_pt_binding(ble_pt_action_t action, ble_pt_frame_t *out) { (void)action; (void)out; return false; }
ble_pt_action_t ble_pt_action_for_frame(const ble_pt_frame_t *frame) { (void)frame; return BLE_PT_ACTION_NONE; }
ble_pt_action_t ble_pt_action_for_usage(uint8_t usage, uint8_t mods) { (void)usage; (void)mods; return BLE_PT_ACTION_NONE; }

// JPEG hardware stubs
#include "jpeg_decoder.h"

esp_err_t esp_jpeg_decode(esp_jpeg_image_cfg_t *cfg, esp_jpeg_image_output_t *img) {
    (void)cfg; (void)img;
    return ESP_FAIL;
}

esp_err_t esp_jpeg_get_image_info(esp_jpeg_image_cfg_t *cfg, esp_jpeg_image_output_t *img) {
    (void)cfg; (void)img;
    return ESP_FAIL;
}

#ifndef ESP_PLATFORM
size_t test_free = 32u * 1024u * 1024u;
size_t test_largest = 32u * 1024u * 1024u;
int test_hw_error = 0;
int test_alloc_fail = 0;
int test_allocs = 0;
int test_engines = 0;
int test_hw_calls = 0;
int64_t test_time = 0;
int64_t test_time_step = 0;
#endif
