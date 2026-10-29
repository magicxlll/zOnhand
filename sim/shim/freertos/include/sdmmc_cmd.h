#ifndef _SDMMC_CMD_H_
#define _SDMMC_CMD_H_

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sdmmc_card_t {
    uint32_t capacity;
    uint32_t sector_size;
} sdmmc_card_t;

#ifdef __cplusplus
}
#endif

#endif // _SDMMC_CMD_H_
