#include "esp_random.h"
#include "esp_chip_info.h"
#include "esp_app_desc.h"
#include <cstdlib>
#include <cstring>
#include <random>

extern "C" {

uint32_t esp_random(void) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<uint32_t> dis;
    return dis(gen);
}

void esp_fill_random(void *buf, size_t len) {
    if (!buf || len == 0) return;
    uint8_t *p = static_cast<uint8_t *>(buf);
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<uint32_t> dis;
    size_t i = 0;
    while (i + 4 <= len) {
        uint32_t r = dis(gen);
        std::memcpy(p + i, &r, 4);
        i += 4;
    }
    if (i < len) {
        uint32_t r = dis(gen);
        std::memcpy(p + i, &r, len - i);
    }
}

void esp_chip_info(esp_chip_info_t *out_info) {
    if (!out_info) return;
    out_info->model = CHIP_ESP32S3;
    out_info->features = 0;
    out_info->cores = 2;
    out_info->revision = 1;
}

const esp_app_desc_t *esp_app_get_description(void) {
    static esp_app_desc_t desc = {
        .magic_word = 0xABCD5432,
        .secure_version = 0,
        .reserv1 = {0, 0},
        .version = "1.0.0-sim",
        .project_name = "Metalio-E-Ink4-Plus",
        .time = "12:00:00",
        .date = "2026-10-29",
        .idf_ver = "v6.1-sim",
        .app_elf_sha256 = {0},
        .reserv2 = {0}
    };
    return &desc;
}

} // extern "C"
