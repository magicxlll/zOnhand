#define SIM_VFS_NO_HOOK 1
#include "sim_vfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include "esp_err.h"
#include "read_pico_sd.h"

#undef fopen
#undef open
#undef opendir
#undef stat
#undef access
#undef remove
#undef unlink
#undef mkdir
#undef rename
#undef rmdir

static char s_storage_root[1024] = "./storage";

void sim_vfs_init(const char *storage_root) {
    if (storage_root && *storage_root) {
        snprintf(s_storage_root, sizeof(s_storage_root), "%s", storage_root);
    }
    char buf[1024];
    snprintf(buf, sizeof(buf), "%s/sdcard", s_storage_root);
    mkdir(buf, 0755);
    snprintf(buf, sizeof(buf), "%s/sdcard/books", s_storage_root);
    mkdir(buf, 0755);
    snprintf(buf, sizeof(buf), "%s/sdcard/fonts", s_storage_root);
    mkdir(buf, 0755);
    snprintf(buf, sizeof(buf), "%s/sdcard/.metalio_s31", s_storage_root);
    mkdir(buf, 0755);
    snprintf(buf, sizeof(buf), "%s/flash", s_storage_root);
    mkdir(buf, 0755);
    snprintf(buf, sizeof(buf), "%s/flash/books", s_storage_root);
    mkdir(buf, 0755);
}

const char* sim_translate_path(const char* path, char* buffer, size_t buflen) {
    if (!path) return NULL;
    if (strncmp(path, "/sdcard/", 8) == 0) {
        snprintf(buffer, buflen, "%s/sdcard/%s", s_storage_root, path + 8);
        return buffer;
    }
    if (strcmp(path, "/sdcard") == 0) {
        snprintf(buffer, buflen, "%s/sdcard", s_storage_root);
        return buffer;
    }
    if (strncmp(path, "/flash/", 7) == 0) {
        snprintf(buffer, buflen, "%s/flash/%s", s_storage_root, path + 7);
        return buffer;
    }
    if (strcmp(path, "/flash") == 0) {
        snprintf(buffer, buflen, "%s/flash", s_storage_root);
        return buffer;
    }
    return path;
}

FILE* sim_fopen(const char* path, const char* mode) {
    char translated[1024];
    const char *real_path = sim_translate_path(path, translated, sizeof(translated));
    return fopen(real_path, mode);
}

int sim_open(const char* path, int flags, ...) {
    char translated[1024];
    const char *real_path = sim_translate_path(path, translated, sizeof(translated));
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode = (mode_t)va_arg(args, int);
        va_end(args);
        return open(real_path, flags, mode);
    }
    return open(real_path, flags);
}

DIR* sim_opendir(const char* path) {
    char translated[1024];
    const char *real_path = sim_translate_path(path, translated, sizeof(translated));
    return opendir(real_path);
}

int sim_stat(const char* path, struct stat* buf) {
    char translated[1024];
    const char *real_path = sim_translate_path(path, translated, sizeof(translated));
    return stat(real_path, buf);
}

int sim_access(const char* path, int mode) {
    char translated[1024];
    const char *real_path = sim_translate_path(path, translated, sizeof(translated));
    return access(real_path, mode);
}

int sim_remove(const char* path) {
    char translated[1024];
    const char *real_path = sim_translate_path(path, translated, sizeof(translated));
    return remove(real_path);
}

int sim_unlink(const char* path) {
    char translated[1024];
    const char *real_path = sim_translate_path(path, translated, sizeof(translated));
    return unlink(real_path);
}

int sim_mkdir(const char* path, mode_t mode) {
    char translated[1024];
    const char *real_path = sim_translate_path(path, translated, sizeof(translated));
    return mkdir(real_path, mode);
}

int sim_rename(const char* oldpath, const char* newpath) {
    char tr_old[1024];
    char tr_new[1024];
    const char *real_old = sim_translate_path(oldpath, tr_old, sizeof(tr_old));
    const char *real_new = sim_translate_path(newpath, tr_new, sizeof(tr_new));
    return rename(real_old, real_new);
}

int sim_rmdir(const char* path) {
    char translated[1024];
    const char *real_path = sim_translate_path(path, translated, sizeof(translated));
    return rmdir(real_path);
}

// read_pico_sd.h implementations
esp_err_t read_pico_sd_get_info(read_pico_sd_info_t* info) {
    if (info) {
        memset(info, 0, sizeof(*info));
        info->present = true;
        info->mounted = true;
        info->needs_format = false;
        strncpy(info->name, "SDCARD", sizeof(info->name));
        info->capacity_bytes = (uint64_t)32 * 1024 * 1024 * 1024ULL; // 32GB
        info->filesystem_bytes = (uint64_t)32 * 1024 * 1024 * 1024ULL;
        info->free_bytes = (uint64_t)28 * 1024 * 1024 * 1024ULL; // 28GB free
        info->space_valid = true;
        info->space_error = ESP_OK;
        info->error = ESP_OK;
    }
    return ESP_OK;
}

esp_err_t read_pico_sd_refresh_space(void) { return ESP_OK; }
esp_err_t read_pico_sd_start_space_refresh(void) { return ESP_OK; }

bool read_pico_sd_io_ready(void) { return true; }
esp_err_t read_pico_sd_start_probe(void) { return ESP_OK; }
void read_pico_sd_poll_health(void) {}
bool read_pico_sd_needs_recovery(void) { return false; }
esp_err_t read_pico_sd_start_recovery(void) { return ESP_OK; }
esp_err_t read_pico_sd_start_manual_recovery(void) { return ESP_OK; }
esp_err_t read_pico_sd_remount(void) { return ESP_OK; }
esp_err_t read_pico_sd_format(void) { return ESP_OK; }

bool app_download_active(void) { return false; }
bool pico_online_busy(void) { return false; }
bool usb_storage_active(void) { return false; }
void usb_storage_phy_init(void) {}
