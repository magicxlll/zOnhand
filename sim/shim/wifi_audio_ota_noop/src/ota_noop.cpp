#include "ota_noop.h"
#include "esp_ota_ops.h"
#include "esp_app_desc.h"
#include "esp_partition.h"
#include <cstdio>
#include <cstring>

namespace sim {

// Singleton accessed via OtaNoop::GetInstance()

} // namespace sim

extern "C" {

esp_err_t esp_ota_mark_app_valid_cancel_rollback(void) {
    sim::OtaNoop::GetInstance().set_version_marked_valid(true);
    std::fprintf(stderr, "[OTA Shim] esp_ota_mark_app_valid_cancel_rollback() ok\n");
    return ESP_OK;
}

const esp_app_desc_t *esp_ota_get_app_description(void) {
    return esp_app_get_description();
}

const esp_partition_t *esp_partition_find_first(esp_partition_type_t type,
                                               esp_partition_subtype_t subtype,
                                               const char *label) {
    (void)type;
    (void)subtype;
    (void)label;
    static esp_partition_t dummy_part = {
        .type = ESP_PARTITION_TYPE_APP,
        .subtype = ESP_PARTITION_SUBTYPE_APP_FACTORY,
        .address = 0x10000,
        .size = 0x1000000, // 16MB
        .label = "factory",
        .encrypted = false
    };
    return &dummy_part;
}

} // extern "C"
