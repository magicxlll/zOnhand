#ifndef _ESP_RANDOM_H_
#define _ESP_RANDOM_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t esp_random(void);
void esp_fill_random(void *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif // _ESP_RANDOM_H_
