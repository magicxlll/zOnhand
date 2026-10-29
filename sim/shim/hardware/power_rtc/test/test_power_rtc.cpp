#include "sim_power_rtc.h"
#include "bq27220_gauge.h"
#include "pcf8563.h"
#include "metalio_rtc.h"
#include "metalio_sys_mon.h"
#include "epd_board_metalio_eink4_plus.h"

#include <cassert>
#include <chrono>
#include <cstdio>
#include <thread>

int main() {
    std::printf("====================================================\n");
    std::printf("[TEST] BQ27220 Fuel Gauge & PCF8563 RTC Shim\n");
    std::printf("====================================================\n");

    // -----------------------------------------------------------------------
    // [1] BQ27220 Fuel Gauge Tests
    // -----------------------------------------------------------------------
    std::printf("[1] Testing BQ27220 Fuel Gauge...\n");
    auto& gauge = Bq27220Gauge::GetInstance();
    bool begin_ok = gauge.Begin(nullptr);
    assert(begin_ok && "BQ27220 Begin() must succeed");
    assert(gauge.IsReady() && "BQ27220 must be ready");

    int level = 0;
    bool charging = false, discharging = false;
    uint16_t mv = 0;
    int16_t ma = 0;

    // Check default baseline (70%, 3850mV, discharging)
    sim::hw::ResetBatteryDefault();
    assert(gauge.GetBatteryLevel(level, charging, discharging));
    assert(level == 70 && "Default battery level must be 70%");
    assert(!charging && discharging && "Default state must be discharging");

    assert(gauge.ReadVoltageMv(mv) && mv == 3850);
    assert(gauge.ReadCurrentMa(ma) && ma < 0);
    std::printf("    Default battery: level=%d%%, voltage=%umV, charging=%d, current=%dma\n",
                level, mv, charging, ma);

    // Test dynamic update
    sim::hw::SetBattery(92, 4180, true, false);
    assert(gauge.GetBatteryLevel(level, charging, discharging));
    assert(level == 92 && charging && !discharging);
    assert(gauge.ReadVoltageMv(mv) && mv == 4180);
    assert(gauge.ReadCurrentMa(ma) && ma > 0);
    std::printf("    Updated battery: level=%d%%, voltage=%umV, charging=%d, current=%dma\n",
                level, mv, charging, ma);
    std::printf("    BQ27220 Fuel Gauge tests PASSED!\n");

    // -----------------------------------------------------------------------
    // [2] PCF8563 RTC Tests
    // -----------------------------------------------------------------------
    std::printf("[2] Testing PCF8563 RTC...\n");
    auto& rtc = Pcf8563::GetInstance();
    bool rtc_begin_ok = rtc.Begin(nullptr);
    assert(rtc_begin_ok && "PCF8563 Begin() must succeed");
    assert(rtc.IsReady() && "PCF8563 must be ready");

    struct tm time_out = {};
    bool valid = false;
    assert(rtc.GetTime(time_out, &valid) && valid);
    std::printf("    Host local RTC time: %04d-%02d-%02d %02d:%02d:%02d\n",
                time_out.tm_year + 1900, time_out.tm_mon + 1, time_out.tm_mday,
                time_out.tm_hour, time_out.tm_min, time_out.tm_sec);
    assert(time_out.tm_year >= 124 && "RTC year must be >= 2024");

    // Custom time injection
    struct tm custom = {};
    custom.tm_year = 2026 - 1900;
    custom.tm_mon = 9; // October (0-indexed)
    custom.tm_mday = 29;
    custom.tm_hour = 15;
    custom.tm_min = 45;
    custom.tm_sec = 0;
    sim::hw::SetRtcTime(custom);

    struct tm read_back = {};
    assert(rtc.GetTime(read_back, &valid) && valid);
    assert(read_back.tm_year == 126 && read_back.tm_mon == 9 && read_back.tm_mday == 29);
    assert(read_back.tm_hour == 15 && read_back.tm_min == 45);
    std::printf("    Custom RTC time verified: %04d-%02d-%02d %02d:%02d:%02d\n",
                read_back.tm_year + 1900, read_back.tm_mon + 1, read_back.tm_mday,
                read_back.tm_hour, read_back.tm_min, read_back.tm_sec);
    std::printf("    PCF8563 RTC tests PASSED!\n");

    // -----------------------------------------------------------------------
    // [3] MetalioRtc HAL Integration
    // -----------------------------------------------------------------------
    std::printf("[3] Testing MetalioRtc HAL...\n");
    MetalioRtc_Init();
    assert(MetalioRtc_Ok() && "MetalioRtc_Ok() must return true");
    assert(!MetalioRtc_IrqPending() && "MetalioRtc_IrqPending() must be false initially");
    std::printf("    MetalioRtc HAL tests PASSED!\n");

    // -----------------------------------------------------------------------
    // [4] System Monitor Integration Test
    // -----------------------------------------------------------------------
    std::printf("[4] Starting MetalioPeriph_StartSystemMonitor()...\n");
    MetalioPeriph_StartSystemMonitor();
    std::printf("    Waiting 3500ms for sys_mon background logging task...\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(3500));
    std::printf("    System Monitor iteration executed cleanly!\n");

    std::printf("====================================================\n");
    std::printf("[TEST] ALL Fuel Gauge & RTC CHECKS PASSED 100%%\n");
    std::printf("====================================================\n");

    return 0;
}
