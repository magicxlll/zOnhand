#ifndef _DRIVER_SDMMC_HOST_H_
#define _DRIVER_SDMMC_HOST_H_

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void *sdmmc_host_t;

typedef struct {
    int width;
    int flags;
} sdmmc_slot_config_t;

#ifdef __cplusplus
}
#endif

#endif // _DRIVER_SDMMC_HOST_H_
