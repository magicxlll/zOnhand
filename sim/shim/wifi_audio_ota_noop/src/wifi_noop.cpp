#include "wifi_noop.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include <cstdio>
#include <cstring>

namespace sim {

void WifiNoop::StartNetwork() {
    // In simulator, no-op by default: offline mode
    std::fprintf(stderr, "[WiFi Shim] StartNetwork() no-op (offline mode)\n");
}

} // namespace sim

static wifi_mode_t s_wifi_mode = WIFI_MODE_NULL;
static wifi_config_t s_wifi_sta_cfg = {};
static wifi_config_t s_wifi_ap_cfg = {};

extern "C" {

esp_err_t esp_wifi_init(const wifi_init_config_t *config) {
    (void)config;
    return ESP_OK;
}

esp_err_t esp_wifi_deinit(void) {
    return ESP_OK;
}

esp_err_t esp_wifi_set_mode(wifi_mode_t mode) {
    s_wifi_mode = mode;
    return ESP_OK;
}

esp_err_t esp_wifi_get_mode(wifi_mode_t *mode) {
    if (!mode) return ESP_ERR_INVALID_ARG;
    *mode = s_wifi_mode;
    return ESP_OK;
}

esp_err_t esp_wifi_start(void) {
    return ESP_OK;
}

esp_err_t esp_wifi_stop(void) {
    return ESP_OK;
}

esp_err_t esp_wifi_connect(void) {
    return ESP_OK;
}

esp_err_t esp_wifi_disconnect(void) {
    return ESP_OK;
}

esp_err_t esp_wifi_set_config(wifi_interface_t interface, wifi_config_t *conf) {
    if (!conf) return ESP_ERR_INVALID_ARG;
    if (interface == WIFI_IF_STA) {
        s_wifi_sta_cfg = *conf;
    } else {
        s_wifi_ap_cfg = *conf;
    }
    return ESP_OK;
}

esp_err_t esp_wifi_get_config(wifi_interface_t interface, wifi_config_t *conf) {
    if (!conf) return ESP_ERR_INVALID_ARG;
    if (interface == WIFI_IF_STA) {
        *conf = s_wifi_sta_cfg;
    } else {
        *conf = s_wifi_ap_cfg;
    }
    return ESP_OK;
}

esp_err_t esp_wifi_scan_start(const wifi_scan_config_t *config, bool block) {
    (void)config;
    (void)block;
    return ESP_OK;
}

esp_err_t esp_wifi_scan_stop(void) {
    return ESP_OK;
}

esp_err_t esp_wifi_scan_get_ap_num(uint16_t *number) {
    if (!number) return ESP_ERR_INVALID_ARG;
    *number = 0;
    return ESP_OK;
}

esp_err_t esp_wifi_scan_get_ap_records(uint16_t *number, wifi_ap_record_t *ap_records) {
    (void)ap_records;
    if (!number) return ESP_ERR_INVALID_ARG;
    *number = 0;
    return ESP_OK;
}

esp_err_t esp_wifi_get_mac(wifi_interface_t ifx, uint8_t mac[6]) {
    (void)ifx;
    if (!mac) return ESP_ERR_INVALID_ARG;
    mac[0] = 0x02;
    mac[1] = 0x00;
    mac[2] = 0x00;
    mac[3] = 0x00;
    mac[4] = 0x00;
    mac[5] = 0x01;
    return ESP_OK;
}

esp_err_t esp_wifi_set_ps(wifi_ps_type_t type) {
    (void)type;
    return ESP_OK;
}

esp_err_t esp_wifi_set_max_tx_power(int8_t power) {
    (void)power;
    return ESP_OK;
}

esp_err_t esp_netif_init(void) {
    return ESP_OK;
}

esp_err_t esp_netif_deinit(void) {
    return ESP_OK;
}

esp_netif_t *esp_netif_create_default_wifi_sta(void) {
    static int dummy_sta = 1;
    return (esp_netif_t *)&dummy_sta;
}

esp_netif_t *esp_netif_create_default_wifi_ap(void) {
    static int dummy_ap = 2;
    return (esp_netif_t *)&dummy_ap;
}

void esp_netif_destroy_default_wifi(esp_netif_t *netif) {
    (void)netif;
}

esp_err_t esp_netif_get_ip_info(esp_netif_t *netif, esp_netif_ip_info_t *ip_info) {
    (void)netif;
    if (!ip_info) return ESP_ERR_INVALID_ARG;
    ip_info->ip.addr = 0;
    ip_info->netmask.addr = 0;
    ip_info->gw.addr = 0;
    return ESP_OK;
}

} // extern "C"
