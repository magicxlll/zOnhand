#include "esp_random.h"
#include "esp_chip_info.h"
#include "esp_app_desc.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "wifi_noop.h"
#include "audio_noop.h"
#include "ota_noop.h"

#include <cassert>
#include <cstdio>
#include <cstring>

int main() {
    std::printf("[TEST] Testing wifi_audio_ota_noop shim...\n");

    // 1. Random tests
    uint32_t r1 = esp_random();
    uint32_t r2 = esp_random();
    assert(r1 != r2 && "esp_random should produce varying numbers");

    uint8_t buf[16] = {0};
    esp_fill_random(buf, sizeof(buf));
    bool all_zero = true;
    for (size_t i = 0; i < sizeof(buf); ++i) {
        if (buf[i] != 0) all_zero = false;
    }
    assert(!all_zero && "esp_fill_random should fill non-zero random bytes");
    std::printf("  [PASS] Random test passed\n");

    // 2. Chip info & app desc
    esp_chip_info_t chip;
    esp_chip_info(&chip);
    assert(chip.cores == 2 && "Chip info should report 2 cores");

    const esp_app_desc_t *desc = esp_app_get_description();
    assert(desc != nullptr);
    assert(std::strcmp(desc->project_name, "Metalio-E-Ink4-Plus") == 0);
    assert(std::strcmp(desc->version, "1.0.0-sim") == 0);
    std::printf("  [PASS] Chip info & app desc passed (%s v%s)\n", desc->project_name, desc->version);

    // 3. WiFi tests
    assert(esp_wifi_init(nullptr) == ESP_OK);
    assert(esp_wifi_set_mode(WIFI_MODE_STA) == ESP_OK);
    wifi_mode_t mode = WIFI_MODE_NULL;
    assert(esp_wifi_get_mode(&mode) == ESP_OK);
    assert(mode == WIFI_MODE_STA);

    uint8_t mac[6] = {0};
    assert(esp_wifi_get_mac(WIFI_IF_STA, mac) == ESP_OK);
    assert(mac[0] == 0x02 && mac[5] == 0x01);

    uint16_t ap_num = 99;
    assert(esp_wifi_scan_get_ap_num(&ap_num) == ESP_OK);
    assert(ap_num == 0);

    assert(esp_wifi_start() == ESP_OK);
    assert(esp_wifi_stop() == ESP_OK);
    assert(esp_wifi_deinit() == ESP_OK);
    std::printf("  [PASS] WiFi shim passed\n");

    // 4. Netif tests
    assert(esp_netif_init() == ESP_OK);
    esp_netif_t *sta = esp_netif_create_default_wifi_sta();
    assert(sta != nullptr);
    esp_netif_ip_info_t ip_info;
    assert(esp_netif_get_ip_info(sta, &ip_info) == ESP_OK);
    esp_netif_destroy_default_wifi(sta);
    std::printf("  [PASS] Netif shim passed\n");

    // 5. Audio tests
    assert(!sim::AudioNoop::GetInstance().is_initialized());
    MetalioAudio_Init();
    assert(sim::AudioNoop::GetInstance().is_initialized());
    assert(MetalioAudio_ModeReady() == false);
    assert(MetalioAudio_WaitModeReady(100) == false);
    MetalioAudio_SetAmpState(true);
    assert(MetalioAudio_GetAmpState() == true);
    MetalioAudio_SetAmpState(false);
    assert(MetalioAudio_GetAmpState() == false);
    std::printf("  [PASS] Audio shim passed\n");

    // 6. OTA tests
    assert(!sim::OtaNoop::GetInstance().version_marked_valid());
    assert(esp_ota_mark_app_valid_cancel_rollback() == ESP_OK);
    assert(sim::OtaNoop::GetInstance().version_marked_valid());
    const esp_app_desc_t *ota_desc = esp_ota_get_app_description();
    assert(ota_desc == desc);

    const esp_partition_t *part = esp_partition_find_first(ESP_PARTITION_TYPE_APP,
                                                           ESP_PARTITION_SUBTYPE_APP_FACTORY,
                                                           nullptr);
    assert(part != nullptr);
    assert(part->size == 0x1000000);
    std::printf("  [PASS] OTA shim passed\n");

    // 7. WifiNoop singleton
    auto &wn = sim::WifiNoop::GetInstance();
    assert(!wn.IsNetworkReady());
    assert(!wn.IsWifiConfigMode());
    wn.StartNetwork();
    std::printf("  [PASS] WifiNoop singleton passed\n");

    std::printf("[TEST] ALL wifi_audio_ota_noop tests PASSED!\n");
    return 0;
}
