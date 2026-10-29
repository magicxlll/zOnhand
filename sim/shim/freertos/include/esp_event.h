#ifndef _ESP_EVENT_H_
#define _ESP_EVENT_H_

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef const char* esp_event_base_t;
typedef void* esp_event_loop_handle_t;
typedef void (*esp_event_handler_t)(void* event_handler_arg,
                                    esp_event_base_t event_base,
                                    int32_t event_id,
                                    void* event_data);

#define ESP_EVENT_DECLARE_BASE(base) extern esp_event_base_t base
#define ESP_EVENT_DEFINE_BASE(base) esp_event_base_t base = #base

inline esp_err_t esp_event_loop_create_default(void) { return ESP_OK; }
inline esp_err_t esp_event_loop_delete_default(void) { return ESP_OK; }
inline esp_err_t esp_event_post(esp_event_base_t event_base, int32_t event_id,
                                void* event_data, size_t event_data_size, uint32_t ticks_to_wait) {
    (void)event_base; (void)event_id; (void)event_data; (void)event_data_size; (void)ticks_to_wait;
    return ESP_OK;
}

#ifdef __cplusplus
}
#endif

#endif // _ESP_EVENT_H_
