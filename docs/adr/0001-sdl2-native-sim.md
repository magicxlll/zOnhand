# ADR-0001: Trình máy ảo — SDL2 native trên macOS + shim ESP-IDF v6.1 host-sim (giữ pipeline build gốc)

- **Status:** Accepted (2026-10-28)
- **Context:** VM phải chạy fw CloudZao (ESP-IDF v6.1, C/C++, LVGL, target esp32s31) trên macOS, mục tiêu test trước khi flash máy thật.
- **Quyết định:**
  1. **Render/host: SDL2 native** (không WASM/browser, không SwiftUI/AppKit) — đúng model crosspoint-reader/crosspoint-simulator (firmware compile native, SDL2 window, macOS ARM64/x86-64 đã được test).
  2. **Firmware code không sửa** — chỉ link thêm shim layer (display, keys, touch, SD, NVS, RTC, gauge).
  3. **Giữ pipeline build ESP-IDF v6.1 gốc**, thêm `sim/` build target (CMake) bên cạnh — KHÔNG kéo sang PlatformIO (điểm khác với crosspoint-simulator; CloudZao không dùng PlatformIO).
- **Biện luận:**
  - QEMU binary-level loại: không support ESP32-S31 (RISC-V) ở mức binary; fw CloudZao build bằng ESP-IDF (không phải bare-metal), nên source-level shim là con đường thực dụng duy nhất.
  - Web/WASM loại khỏi Phase 1: crosspoint-simulator tự khai báo "Native Windows không hỗ trợ" cho platform desktop của nó; browser build (Emscripten) cho ESP-IDF v6.1 + LVGL + Octal PSRAM là rủi ro cao, không cần cho user (macOS). Mở rộng sau nếu cần.
  - PlatformIO (đúng như crosspoint) loại: đổi build pipeline gốc, mất `patches/esp-idf/` (MPLL/s31_sleep compat) và `sdkconfig` tuned. Nếu shim ESP-IDF host không khả thi (rủi ro B1), escalate user trước khi cân nhắc fork sang PlatformIO.
- **Hệ quả:**
  - Phải viết shim cho API ESP-IDF v6.1 (`freertos`, `esp_log`, `esp_timer`, `driver/gpio`, `i2c_master`, `sdmmc`, `nvs`, `esp_system`) — khác shim Arduino/ESP32 của crosspoint. Khối lượng +10–30% so với fork crosspoint-simulator, nhưng giữ được pipeline build & patch gốc.
  - lvgl (S8) là blocker chính (e-paper 16-gray, partial refresh, dirty rect → SDL2 texture 684×1216).
  - Giới hạn shim (theo crosspoint): framebuffer only, không timing/LUT thật, SD→thư mục host, NVS→file, OTA no-op, crypto/DRM trả error.
