#pragma once
#include <stdio.h>
#include <inttypes.h>

#define ESP_LOGE(tag, format, ...) printf("[E][%s] " format "\n", tag, ##__VA_ARGS__)
#define ESP_LOGW(tag, format, ...) printf("[W][%s] " format "\n", tag, ##__VA_ARGS__)
#define ESP_LOGI(tag, format, ...) printf("[I][%s] " format "\n", tag, ##__VA_ARGS__)
#define ESP_LOGD(tag, format, ...) do {} while(0)
#define ESP_LOGV(tag, format, ...) do {} while(0)
