#!/usr/bin/env bash
# ==============================================================================
# zOnhand Phase 1 Simulator - Unified Test Suite Runner
# ==============================================================================
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/.." && pwd)"

echo "=================================================================="
echo " Running zOnhand Phase 1 Full Verification Suite"
echo " Target: Metalio E-Ink 4 Plus (ESP32-S31, ED047TC2 684x1216)"
echo " Host: macOS Native (SDL2 + LVGL v9.5 + FreeRTOS/NVS/HW Shims)"
echo "=================================================================="

# 1. Build and test Interactive Emulator (T1, T2', T3, T5, T7, T8)
echo ""
echo ">>> [1/5] Building & Testing Interactive Emulator (T2' Shell, Reading, NVS, Standby)..."
make -C "$HERE/ebook-emulator/build-ebook" ebook_test ebook_sim -j"$(sysctl -n hw.ncpu 2>/dev/null || nproc)" >/dev/null
"$HERE/ebook-emulator/build-ebook/bin/ebook_test"
echo "--- Interactive Emulator Test PASSED ---"

# 2. Build and test TCA9555 & Metalio Keys (T4)
echo ""
echo ">>> [2/5] Building & Testing TCA9555 & Physical Keys Shim (T4)..."
cmake -B "$HERE/shim/hardware/tca9555_keys/build" -S "$HERE/shim/hardware/tca9555_keys" >/dev/null
cmake --build "$HERE/shim/hardware/tca9555_keys/build" >/dev/null
"$HERE/shim/hardware/tca9555_keys/build/test_tca9555_keys"
echo "--- TCA9555 & Keys Test PASSED ---"

# 3. Build and test BQ27220 Fuel Gauge & PCF8563 RTC (T6)
echo ""
echo ">>> [3/5] Building & Testing Battery Gauge & RTC Shim (T6)..."
cmake -B "$HERE/shim/hardware/power_rtc/build" -S "$HERE/shim/hardware/power_rtc" >/dev/null
cmake --build "$HERE/shim/hardware/power_rtc/build" >/dev/null
"$HERE/shim/hardware/power_rtc/build/test_power_rtc"
echo "--- Fuel Gauge & RTC Test PASSED ---"

# 4. Build and test Shim Micro-units (FreeRTOS, NVS, WiFi/Audio/OTA)
echo ""
echo ">>> [4/5] Testing Shim Micro-units (FreeRTOS, NVS, Network/Audio/OTA)..."
"$HERE/shim/application_boot/build/freertos_build/test_freertos_shim"
"$HERE/shim/application_boot/build/nvs_build/test_nvs_shim"
"$HERE/shim/application_boot/build/wifi_build/test_wifi_audio_ota_shim"
echo "--- Shim Micro-units Test PASSED ---"

# 5. Build and test Firmware Boot (Application::Start())
echo ""
echo ">>> [5/5] Building & Testing Firmware Boot Sequence (Application::Start)..."
cmake -B "$HERE/shim/application_boot/build" -S "$HERE/shim/application_boot" >/dev/null
cmake --build "$HERE/shim/application_boot/build" >/dev/null
"$HERE/shim/application_boot/build/test_boot_application"
echo "--- Firmware Boot Test PASSED ---"

echo ""
echo "=================================================================="
echo " 🎉 ALL PHASE 1 TESTS PASSED SUCCESSFULLY! (100% GREEN)"
echo "=================================================================="
