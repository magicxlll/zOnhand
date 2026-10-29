#ifndef _ESP_PARTITION_H_
#define _ESP_PARTITION_H_

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t esp_partition_mmap_handle_t;

typedef enum {
    ESP_PARTITION_TYPE_APP = 0x00,
    ESP_PARTITION_TYPE_DATA = 0x01,
    ESP_PARTITION_TYPE_ANY = 0xFF,
} esp_partition_type_t;

typedef enum {
    ESP_PARTITION_SUBTYPE_APP_FACTORY = 0x00,
    ESP_PARTITION_SUBTYPE_APP_OTA_0 = 0x10,
    ESP_PARTITION_SUBTYPE_DATA_NVS = 0x02,
    ESP_PARTITION_SUBTYPE_ANY = 0xFF,
} esp_partition_subtype_t;

typedef struct {
    esp_partition_type_t type;
    esp_partition_subtype_t subtype;
    uint32_t address;
    uint32_t size;
    char label[17];
    bool encrypted;
} esp_partition_t;

typedef void *esp_partition_iterator_t;

inline esp_partition_iterator_t esp_partition_find(esp_partition_type_t type,
                                                   esp_partition_subtype_t subtype,
                                                   const char *label) {
    (void)type; (void)subtype; (void)label;
    return nullptr;
}

inline const esp_partition_t *esp_partition_get(esp_partition_iterator_t iterator) {
    (void)iterator;
    return nullptr;
}

inline esp_partition_iterator_t esp_partition_next(esp_partition_iterator_t iterator) {
    (void)iterator;
    return nullptr;
}

inline void esp_partition_iterator_release(esp_partition_iterator_t iterator) {
    (void)iterator;
}

const esp_partition_t *esp_partition_find_first(esp_partition_type_t type,
                                               esp_partition_subtype_t subtype,
                                               const char *label);

#ifdef __cplusplus
}
#endif

#endif // _ESP_PARTITION_H_
