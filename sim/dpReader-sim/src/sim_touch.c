#include "cst836u.h"
#include <string.h>

static cst836u_touch_t s_touch = {0};

void sim_touch_set(bool touched, uint16_t x, uint16_t y) {
    s_touch.touched = touched;
    s_touch.count = touched ? 1 : 0;
    s_touch.x = x;
    s_touch.y = y;
    s_touch.points[0].active = touched;
    s_touch.points[0].x = x;
    s_touch.points[0].y = y;
}

esp_err_t cst836u_read(cst836u_handle_t handle, cst836u_touch_t *touch) {
    (void)handle;
    if (!touch) return ESP_ERR_INVALID_ARG;
    *touch = s_touch;
    return ESP_OK;
}

esp_err_t cst836u_get_info(cst836u_handle_t handle, cst836u_info_t *info) {
    (void)handle;
    (void)info;
    return ESP_OK;
}
