#ifndef _ESP_OTA_OPS_H_
#define _ESP_OTA_OPS_H_

#include "esp_err.h"
#include "esp_app_desc.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t esp_ota_handle_t;

esp_err_t esp_ota_mark_app_valid_cancel_rollback(void);
const esp_app_desc_t *esp_ota_get_app_description(void);

#ifdef __cplusplus
}
#endif

#endif // _ESP_OTA_OPS_H_
