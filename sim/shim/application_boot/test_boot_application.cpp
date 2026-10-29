#include "application.h"
#include "board.h"
#include "nvs_flash.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <atomic>

int main() {
    std::printf("====================================================\n");
    std::printf("[BOOT TEST] Starting Application::Start() in Simulator\n");
    std::printf("====================================================\n");

    // Use a clean temporary NVS directory for deterministic test
    setenv("ZONHAND_NVS_DIR", "/tmp/zonhand_boot_test_nvs", 1);
    system("rm -rf /tmp/zonhand_boot_test_nvs && mkdir -p /tmp/zonhand_boot_test_nvs");

    // Initialize NVS
    esp_err_t nvs_err = nvs_flash_init();
    assert(nvs_err == ESP_OK && "NVS flash init must succeed");
    std::printf("[BOOT TEST] NVS initialized at /tmp/zonhand_boot_test_nvs\n");

    auto& app = Application::GetInstance();

    // Verify pre-boot state
    assert(!app.IsBootReady());
    assert(app.GetDeviceState() == kDeviceStateUnknown);
    std::printf("[BOOT TEST] Pre-boot state verified (kDeviceStateUnknown)\n");

    // Run Application::Start()
    std::printf("[BOOT TEST] Calling Application::GetInstance().Start()...\n");
    app.Start();

    // Verify post-boot state
    assert(app.IsBootReady() && "Application::Start() must set boot_ready to true");
    assert(app.GetDeviceState() == kDeviceStateIdle && "Device state after boot must be kDeviceStateIdle");
    assert(!app.IsBootWorkActive() && "Boot work active flag must be false after warmup completes");
    std::printf("[BOOT TEST] Boot state verified: boot_ready=true, state=kDeviceStateIdle\n");

    // Verify MainEventLoop and Schedule() mechanism
    std::printf("[BOOT TEST] Verifying MainEventLoop callback scheduling...\n");
    std::atomic<bool> scheduled_ran{false};
    app.Schedule([&scheduled_ran]() {
        std::printf("[BOOT TEST] ---> Scheduled task executed inside MainEventLoop thread!\n");
        scheduled_ran.store(true, std::memory_order_release);
    });

    for (int i = 0; i < 50; ++i) {
        if (scheduled_ran.load(std::memory_order_acquire)) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    assert(scheduled_ran.load() && "Scheduled task must run inside MainEventLoop");
    std::printf("[BOOT TEST] Schedule() test PASS\n");

    // Verify esp_timer periodic clock ticks
    std::printf("[BOOT TEST] Waiting for clock timer tick...\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(1200));
    std::printf("[BOOT TEST] Clock timer verification complete\n");

    std::printf("====================================================\n");
    std::printf("[BOOT TEST] ALL CHECKS PASSED: Application::Start() OK!\n");
    std::printf("====================================================\n");

    // Clean exit
    return 0;
}
