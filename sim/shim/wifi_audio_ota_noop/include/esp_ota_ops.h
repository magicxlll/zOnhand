#ifndef _ESP_OTA_OPS_H_
#define _ESP_OTA_OPS_H_

#include "esp_err.h"
#include "esp_app_desc.h"
#include "esp_partition.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t esp_ota_handle_t;

esp_err_t esp_ota_mark_app_valid_cancel_rollback(void);
const esp_app_desc_t *esp_ota_get_app_description(void);

inline const esp_partition_t *esp_ota_get_running_partition(void) {
    static esp_partition_t running_part = {
        .type = ESP_PARTITION_TYPE_APP,
        .subtype = ESP_PARTITION_SUBTYPE_APP_OTA_0,
        .address = 0x20000,
        .size = 0x300000,
        .label = "ota_0",
        .encrypted = false
    };
    return &running_part;
}

#ifdef __cplusplus
}
#endif

#endif // _ESP_OTA_OPS_H_
