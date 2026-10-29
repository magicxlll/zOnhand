#include "sim_hardware.h"
#include "metalio_keys.h"
#include "epd_board_metalio_eink4_plus.h"

#include <cassert>
#include <chrono>
#include <cstdio>
#include <thread>

int main() {
    std::printf("====================================================\n");
    std::printf("[TEST] TCA9555 & MetalioKeys Physical Buttons Shim\n");
    std::printf("====================================================\n");

    sim::hw::ResetHardwareState();

    std::printf("[1] Initializing MetalioKeys_Init()...\n");
    MetalioKeys_Init();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    assert(MetalioKeys_IsReady() && "MetalioKeys must be ready after init");
    assert(!MetalioKeys_IsDown(0) && "BOOT key must initially be UP");
    assert(!MetalioKeys_IsDown(1) && "VOL+ key must initially be UP");
    assert(!MetalioKeys_IsDown(2) && "VOL- key must initially be UP");
    assert(!MetalioKeys_IsDown(3) && "POWER key must initially be UP");
    std::printf("    Initial idle states verified (all keys UP)\n");

    // Clear any initial edges
    while (MetalioKeys_TakeEdge()) {}

    std::printf("[2] Testing VOL+ (TCA P10) press and release...\n");
    sim::hw::SetKey(sim::hw::Key::kVolUp, true);
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    assert(MetalioKeys_IsDown(1) && "VOL+ must be detected DOWN");
    assert(MetalioKeys_TakeEdge() && "Edge must be queued for VOL+ down");

    sim::hw::SetKey(sim::hw::Key::kVolUp, false);
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    assert(!MetalioKeys_IsDown(1) && "VOL+ must be detected UP");
    assert(MetalioKeys_TakeEdge() && "Edge must be queued for VOL+ up");
    std::printf("    VOL+ test PASSED!\n");

    std::printf("[3] Testing VOL- (TCA P11) press and release...\n");
    sim::hw::SetKey(sim::hw::Key::kVolDown, true);
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    assert(MetalioKeys_IsDown(2) && "VOL- must be detected DOWN");
    assert(MetalioKeys_TakeEdge() && "Edge must be queued for VOL- down");

    sim::hw::SetKey(sim::hw::Key::kVolDown, false);
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    assert(!MetalioKeys_IsDown(2) && "VOL- must be detected UP");
    assert(MetalioKeys_TakeEdge() && "Edge must be queued for VOL- up");
    std::printf("    VOL- test PASSED!\n");

    std::printf("[4] Testing POWER (TCA P17) short press (click)...\n");
    sim::hw::SetKey(sim::hw::Key::kPower, true);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    assert(MetalioKeys_IsDown(3) && "POWER must be detected DOWN");

    sim::hw::SetKey(sim::hw::Key::kPower, false);
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    assert(!MetalioKeys_IsDown(3) && "POWER must be detected UP");
    assert(MetalioKeys_TakePowerClick() && "Short press must register as PowerClick");
    assert(!MetalioKeys_TakePowerClick() && "PowerClick must be one-shot consumed");
    std::printf("    POWER short click test PASSED!\n");

    std::printf("[5] Testing POWER long press (3000ms -> shutdown pulse)...\n");
    sim::hw::SetKey(sim::hw::Key::kPower, true);
    std::printf("    Holding POWER down for 3100ms...\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(3100));
    assert(MetalioKeys_TakeShutdownPulse() && "POWER long press >=3000ms must trigger shutdown pulse");
    sim::hw::SetKey(sim::hw::Key::kPower, false);
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    std::printf("    POWER long press shutdown test PASSED!\n");

    std::printf("[6] Testing BOOT button (GPIO61, interrupt driven)...\n");
    sim::hw::SetKey(sim::hw::Key::kBoot, true);
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    assert(MetalioKeys_IsDown(0) && "BOOT key must be detected DOWN");
    assert(MetalioKeys_TakeEdge() && "Edge must be queued for BOOT down");

    sim::hw::SetKey(sim::hw::Key::kBoot, false);
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    assert(!MetalioKeys_IsDown(0) && "BOOT key must be detected UP");
    assert(MetalioKeys_TakeEdge() && "Edge must be queued for BOOT up");
    std::printf("    BOOT key test PASSED!\n");

    std::printf("[7] Testing Motor and Shutdown pulse HAL helpers...\n");
    epd_board_metalio_eink4_plus_motor_set(true);
    assert(sim::hw::GetMotorState() && "Motor state must be ON");
    epd_board_metalio_eink4_plus_motor_set(false);
    assert(!sim::hw::GetMotorState() && "Motor state must be OFF");

    epd_board_metalio_eink4_plus_pwr_key_pulse_train();
    assert(sim::hw::GetShutdownPulseCount() == 1 && "Shutdown pulse count must be 1");
    std::printf("    HAL helpers test PASSED!\n");

    std::printf("====================================================\n");
    std::printf("[TEST] ALL TCA9555 & MetalioKeys CHECKS PASSED 100%%\n");
    std::printf("====================================================\n");

    return 0;
}
