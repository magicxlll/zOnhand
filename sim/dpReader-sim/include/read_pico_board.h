#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define READ_PICO_PRODUCT_NAME "dpReader"
#define READ_PICO_DEVICE_NAME "dpReader"

void read_pico_i2c_census_take(void);
bool read_pico_epd_release_drive(void);
bool read_pico_sd_present(void);
void read_pico_epd_restore_lcd(void);

#ifdef __cplusplus
}
#endif
