#pragma once
#include <stdint.h>
#include <stddef.h>
#include <zlib.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline uint32_t esp_rom_crc32_le(uint32_t crc, const uint8_t *buf, size_t len) {
    return (uint32_t)crc32(crc, buf, (uInt)len);
}

#ifdef __cplusplus
}
#endif
