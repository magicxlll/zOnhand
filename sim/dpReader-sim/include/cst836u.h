#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* cst836u_handle_t;
#define CST836U_MAX_POINTS 2

typedef struct {
    bool active;
    uint16_t x;
    uint16_t y;
} cst836u_point_t;

typedef struct {
    bool touched;
    uint8_t count;
    uint16_t x;
    uint16_t y;
    cst836u_point_t points[2];
    uint8_t raw[15];
} cst836u_touch_t;

typedef struct {
    int unused;
} cst836u_info_t;

esp_err_t cst836u_read(cst836u_handle_t handle, cst836u_touch_t *touch);
esp_err_t cst836u_get_info(cst836u_handle_t handle, cst836u_info_t *info);

// Simulator injection functions
void sim_touch_set(bool touched, uint16_t x, uint16_t y);

#ifdef __cplusplus
}
#endif
