#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "nvs.h"
#include "nvs_flash.h"

int main() {
    printf("=== Running NVS File-Based Shim Unit Tests ===\n");
    // Use a test directory
    setenv("ZONHAND_NVS_DIR", "/tmp/zonhand_nvs_test", 1);

    assert(nvs_flash_init() == ESP_OK);

    // Test 1: Write entries to namespace "test_ns"
    printf("[1] Writing key-value pairs...\n");
    nvs_handle_t h = 0;
    assert(nvs_open("test_ns", NVS_READWRITE, &h) == ESP_OK);
    assert(h != 0);

    assert(nvs_set_str(h, "device_name", "Metalio Plus") == ESP_OK);
    assert(nvs_set_i32(h, "font_size", 30) == ESP_OK);
    assert(nvs_set_u8(h, "wifi_enabled", 1) == ESP_OK);

    uint8_t blob_in[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    assert(nvs_set_blob(h, "magic_blob", blob_in, sizeof(blob_in)) == ESP_OK);

    assert(nvs_commit(h) == ESP_OK);
    nvs_close(h);
    printf("    Write & commit OK.\n");

    // Test 2: Re-open and verify persistence from disk
    printf("[2] Re-opening and verifying persistence...\n");
    nvs_handle_t h2 = 0;
    assert(nvs_open("test_ns", NVS_READONLY, &h2) == ESP_OK);
    assert(h2 != 0);

    // String length query
    size_t str_len = 0;
    assert(nvs_get_str(h2, "device_name", nullptr, &str_len) == ESP_OK);
    assert(str_len == strlen("Metalio Plus") + 1);

    char str_buf[64] = {0};
    assert(nvs_get_str(h2, "device_name", str_buf, &str_len) == ESP_OK);
    assert(strcmp(str_buf, "Metalio Plus") == 0);
    printf("    String: '%s' OK.\n", str_buf);

    int32_t val_i32 = 0;
    assert(nvs_get_i32(h2, "font_size", &val_i32) == ESP_OK);
    assert(val_i32 == 30);
    printf("    Int32: %d OK.\n", val_i32);

    uint8_t val_u8 = 0;
    assert(nvs_get_u8(h2, "wifi_enabled", &val_u8) == ESP_OK);
    assert(val_u8 == 1);
    printf("    Uint8: %u OK.\n", val_u8);

    size_t blob_len = 0;
    assert(nvs_get_blob(h2, "magic_blob", nullptr, &blob_len) == ESP_OK);
    assert(blob_len == 4);
    uint8_t blob_out[4] = {0};
    assert(nvs_get_blob(h2, "magic_blob", blob_out, &blob_len) == ESP_OK);
    assert(memcmp(blob_in, blob_out, 4) == 0);
    printf("    Blob: %02X %02X %02X %02X OK.\n", blob_out[0], blob_out[1], blob_out[2], blob_out[3]);

    nvs_close(h2);

    // Test 3: Key erase
    printf("[3] Testing key erase...\n");
    assert(nvs_open("test_ns", NVS_READWRITE, &h) == ESP_OK);
    assert(nvs_erase_key(h, "font_size") == ESP_OK);
    assert(nvs_commit(h) == ESP_OK);
    nvs_close(h);

    assert(nvs_open("test_ns", NVS_READONLY, &h2) == ESP_OK);
    assert(nvs_get_i32(h2, "font_size", &val_i32) == ESP_ERR_NVS_NOT_FOUND);
    nvs_close(h2);
    printf("    Key erase OK.\n");

    // Test 4: Flash erase
    printf("[4] Testing nvs_flash_erase...\n");
    assert(nvs_flash_erase() == ESP_OK);
    assert(nvs_open("test_ns", NVS_READONLY, &h2) == ESP_OK);
    assert(nvs_get_str(h2, "device_name", str_buf, &str_len) == ESP_ERR_NVS_NOT_FOUND);
    nvs_close(h2);
    printf("    Flash erase OK.\n");

    printf("=== All NVS Shim Tests PASSED! ===\n");
    return 0;
}
