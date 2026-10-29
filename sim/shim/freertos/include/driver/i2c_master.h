#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int i2c_port_t;
#define I2C_NUM_0 0
#define I2C_NUM_1 1

typedef void* i2c_master_dev_handle_t;
typedef void* i2c_master_bus_handle_t;

#ifdef __cplusplus
}
#endif
