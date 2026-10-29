#pragma once
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline uint32_t esp_random(void) {
    return (uint32_t)arc4random();
}

static inline void esp_fill_random(void *buf, size_t len) {
    arc4random_buf(buf, len);
}

#ifdef __cplusplus
}
#endif
