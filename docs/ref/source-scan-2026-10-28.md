# Ref: crosspoint-simulator (blueprint kiến trúc VM)

Source: `https://github.com/crosspoint-reader/crosspoint-simulator` (WebFetch 2026-10-28).

- Firmware **compile native C/C++**, render trong **SDL2 window** — không WASM, không SwiftUI. macOS (x86-64 & ARM64) + Linux/WSL đã test; native Windows không hỗ trợ (dùng WSL); browser/iOS/Android không hỗ trợ.
- Firmware chạy **không sửa code**: thêm 1 shim library (display, buttons, SD→`./fs_/`, NVS→`./fs_/.simulator-nvs/`, Wi-Fi→host `curl` + WebServer 127.0.0.1:8080, WebSockets :8081, OTA no-op).
- Pipeline: `lib_dep` simulator + `[env:simulator]` trong `platformio.ini` → `pio run -e simulator` (crosspoint dùng PlatformIO/Arduino — **khác zOnhand: CloudZao dùng ESP-IDF v6.1 CMake**, nên zOnhand viết host-sim layer riêng, giữ pipeline gốc).
- Giới hạn shim: framebuffer only (không timing/LUT/ghosting), JPEG/PNG/QR shim, DRM (AES/RSA/PKCS#12) trả error, `GfxRenderer::setOrientation()` hook resize window.
- Cấu trúc tham khảo: `src/` (shim+SDL2), `docs/` (per-device profile: `metalio-eink4.md` **tồn tại** — thiết bị cùng họ!), `run_simulator.py` hook, `FORKING.md` (guide cho fork HAL khác nhau), `sample-platformio-macos.ini`.
- Ghi chú dùng được: profile `metalio-eink4` trong docs của họ chứng minh thiết bị Metalio (phi Plus, ESP32-S3) từng được support → có tham khảo cách đặt device capability flags.

# Ref: reader-simulator (PewPewzxc) — "search" thực chất là gì

Source: `https://github.com/PewPewzxc/reader-simulator` (WebFetch 2026-10-28).

- Simulator emul 3 model Xteink, chạy community firmware (VARAGH, CrossPoint, CrossInk, CrossMux, YACP...) qua **WASM (browser) + WPF (Windows)**; "the firmware's own source is never modified."
- **"Search" KHÔNG phải UI của simulator** — là feature của **firmware đang chạy** (từ điển online, book catalogue). Simulator chỉ cung cấp: (a) browse/upload SD card (IndexedDB/local folder), (b) "simulated Wi-Fi" cho firmware gọi internet thật; service block web-origin thì "coi như offline."
- → Điểm học cho zOnhand: VM là **hardware abstraction + network path**; mọi "tính năng" (search, dictionary, catalogue) thuộc về fw. Khi fork Phase 2/3 thêm tính năng, phải test trên sim đúng cơ chế này.
- Pipeline CI: GitHub Actions build weekly (browser) + on-demand (Windows), smoke test headless trước khi publish; `build.json` ghi source commit.

# Ref: Metalio E-Ink4 Plus — CloudZao (nền tảng build)

Source: `https://github.com/CloudZao/Metalio-E-INK4-Plus` (WebFetch 2026-10-28 + clone local `vendor/cloudzao/`).

- HW: ESP32-S31 (RISC-V), 16MB flash, Octal PSRAM @200MHz; EPD ED047TC2 4.7" (684×1216 portrait, parallel EPDiy + TPS65185); touch FT6336U I2C 0x38; SD SDMMC 4-bit; BT audio BH1098G (UART AT 38/39, I2S 42–45); accel SC7A20H; RTC PCF8563; gauge BQ27220; TCA9555 expander 0x20; USB MSC.
- FW stack: **ESP-IDF v6.1, CMake, C/C++, LVGL** (e-paper optimized: full/partial refresh, no scroll). Derived from xiaozhi-esp32.
- Build: `idf.py build` (sdkconfig tuned, đừng edit tùy tiện); release: `./merge_firmware.sh` (merge `use_font/font.fontpack` vào `font_data`, ra full .bin); flash: `idf.py -p /dev/ttyACM0 flash monitor`.
- Features gốc (để scope Phase 2/3): AI voice "Baiwen" (ASR→LLM→TTS over WebSocket/MQTT, A2UI paged), reader (3×3 cover grid → detail → body, .ebook/.epub/.txt, volume keys page), wallpaper gallery, daily checklist + cloud sync, frontlight dual cool/warm, AP/USB transfer, multi-tier CPU scaling (80/160/240MHz) + light-sleep standby, BT audio 3 modes.
- `patches/esp-idf/` + `s31_sleep/` module (MPLL/PSRAM sleep compat) — **chỉ apply khi build flash thật** (spec B3).
- Sibling note: E-Ink 4 (không Plus) dùng ESP32-S3, 3.97" SPI panel, CST816S — pin map **không dùng chung** với Plus.

# Ref: RickyOS install page (rickyaistudio.com)

Source: `https://rickyaistudio.com/install.html` (WebFetch 2026-10-28).

- Target: MindReset Read Pico (RDP-G01-W, ESP32-S3, 16MB flash, 684×1216), **Metalio E-Ink4 Plus**, EEGO A4, "RickyVerse."
- **Không có link download .bin thật trên trang** (button trỏ `#`, version "RickyOS 1.2" ghi "正式固件待发布" = chính thức chưa phát hành). File `RickyOS-Plus-1.2.9-full.bin` (16.7MB ≈ đúng 1 image flash 16MB) do user cung cấp sẵn.
- Install flow (trang): USB data cable → trang auto-check partition/boot (không đụng SD) → backup full 16MB flash (5–10 phút, tùy chọn) → confirm device → install; mỗi segment write được device tự verify.
- Features công khai (dùng cho REA spec, §7): "小狗伴读模式" (dog-companion reading mode), auto-detect factory vs flashed system, SD không bị đụng, local-only (không upload), op-log export ("保存记录").
- ⚠️ Định vị đúng: RickyOS = "spec sống" (đã chạy tốt máy thật) — nguồn feature discovery cho fork, **không phải** binary chạy trên VM.
