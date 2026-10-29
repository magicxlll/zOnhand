#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int wl_handle_t;
#define WL_INVALID_HANDLE -1

typedef struct {
    int max_files;
    size_t allocation_unit_size;
    bool format_if_mount_failed;
} esp_vfs_fat_mount_config_t;

static inline esp_err_t esp_vfs_fat_spiflash_mount_rw_wl(const char* base_path, const char* partition_label, const esp_vfs_fat_mount_config_t* mount_config, wl_handle_t* wl_handle) {
    (void)base_path; (void)partition_label; (void)mount_config;
    if (wl_handle) *wl_handle = 1;
    return ESP_OK;
}

static inline esp_err_t esp_vfs_fat_spiflash_unmount_rw_wl(const char* base_path, wl_handle_t wl_handle) {
    (void)base_path; (void)wl_handle;
    return ESP_OK;
}

static inline esp_err_t esp_vfs_fat_info(const char* base_path, uint64_t* out_total_bytes, uint64_t* out_free_bytes) {
    (void)base_path;
    if (out_total_bytes) *out_total_bytes = (uint64_t)32 * 1024 * 1024 * 1024ULL;
    if (out_free_bytes) *out_free_bytes = (uint64_t)28 * 1024 * 1024 * 1024ULL;
    return ESP_OK;
}

#ifdef __cplusplus
}
#endif
