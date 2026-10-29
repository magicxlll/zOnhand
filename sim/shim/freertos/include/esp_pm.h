#ifndef _ESP_PM_H_
#define _ESP_PM_H_

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* esp_pm_lock_handle_t;
typedef enum {
    ESP_PM_CPU_FREQ_MAX,
    ESP_PM_APB_FREQ_MAX,
    ESP_PM_NO_LIGHT_SLEEP,
} esp_pm_lock_type_t;

inline esp_err_t esp_pm_lock_create(esp_pm_lock_type_t lock_type, int arg, const char* name, esp_pm_lock_handle_t* out_handle) {
    (void)lock_type; (void)arg; (void)name;
    if (out_handle) *out_handle = (void*)0x1;
    return ESP_OK;
}
inline esp_err_t esp_pm_lock_delete(esp_pm_lock_handle_t handle) { (void)handle; return ESP_OK; }
inline esp_err_t esp_pm_lock_acquire(esp_pm_lock_handle_t handle) { (void)handle; return ESP_OK; }
inline esp_err_t esp_pm_lock_release(esp_pm_lock_handle_t handle) { (void)handle; return ESP_OK; }

#ifdef __cplusplus
}
#endif

#endif // _ESP_PM_H_
