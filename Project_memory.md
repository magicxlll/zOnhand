# zOnhand — Project Memory

> File này là memory cục bộ của dự án. Mọi phiên bắt đầu phải đọc file này + `docs/` để nắm context. Cập nhật liên tục sau mỗi debug.

## 1. Project Summary

**zOnhand** = trình máy ảo cho máy đọc sách **Metalio E-INK 4.7 Plus** (ESP32-S31 RISC-V, 16MB flash, EPD ED047TC2 4.7" e-paper, 684×1216).

Mục tiêu (chốt 2026-10-28, sau phỏng vấn intent):
1. **VM kiểu crosspoint-simulator**: compile source fw **CloudZao/Metalio-E-INK4-Plus** (ESP-IDF v6.1, LVGL, MIT) native, chạy trong SDL2 trên **macOS** — firmware không sửa code, shim phần cứng.
2. **RickyOS-Plus-1.2.9-full.bin** (đã có trong repo, 16.7MB): binary đóng → không chạy được trên VM. Vai trò: "spec sống" — REA phân tích (strings/regions/handoffs) + demo công khai → **đề xuất danh sách tính năng, user review từng mục** → spec + test scenarios chạy trên VM.
3. **Step 2 (sau khi VM đạt):** fork CloudZao thành nhánh mới, custom tính năng kiểu RickyOS, mỗi tính năng verify bằng test scenario trong VM trước.

## 2. Chốt kỹ thuật

- Kiến trúc: **SDL2 native + shim ESP-IDF host-sim** (giữ pipeline build ESP-IDF v6.1 gốc, thêm env host-sim bên cạnh). Không WASM/browser. Không QEMU binary-level.
- Pha 1 (đang làm): boot → hiển thị EPD (full + partial refresh, dither) → phím vật lý (3 trái + 2 phải + núm xoay, map GPIO thật từ `main/boards/metalio-e-ink4-plus/config.h`) + touch FT6336 (I2C 0x38) → đọc sách (lưới bìa 3×3 → chi tiết → nội dung; .epub/.txt/.ebook) → settings tối thiểu + NVS/RTC/gauge.
- Tiêu chí "thông" pha 1: cùng commit → cùng hành vi với máy thật. File SD thật đặt vào thư mục sim đọc được.
- Pha 2: Wi-Fi/cloud/USB MSC. Pha 3: AI voice, BT audio, frontlight, checklist.
- Out of scope hiện tại: chạy binary .bin đóng; Web/iOS/Windows.
- **User chưa có máy thật** → baseline hành vi = docs CloudZao + code + bằng chứng RickyOS (đã chạy tốt trên máy thật).

## 3. HW Metalio E-INK 4 Plus (để shim đúng)

| Linh kiện | Chi tiết | Ghi chú shim |
|---|---|---|
| MCU | ESP32-S31 (RISC-V), 16MB flash, Octal PSRAM @200MHz | QEMU không support → shim source-level |
| Display | ED047TC2, 4.7", 1216×684 native (UI 684×1216 portrait), EPDiy + TPS65185 PMIC | framebuffer only, full/partial refresh, dither |
| Touch | FT6336U capacitive, I2C 0x38 | map tọa độ host→device |
| Phím (đã verify source 2026-10-28) | **BOOT** (nút cảm ứng tròn dưới cạnh trái, "Physical Buttons") = GPIO61, IRQ; **VOL+/VOL-/POWER** (cạnh phải) = TCA9555 P10/P11/P17 (I2C 0x20); P13 = pulse lệnh tắt máy (OUTPUT, không phải nút); TCA INT = GPIO2 | **KHÔNG có núm xoay** — đối chiếu với ảnh marketing: 4 nút cảm ứng/physical = BOOT+VOL+POWER; "núm giữa" 2 cụm phải trên ảnh = P13 pulse = không bấm được. Virtual keys (vạch 1/2/3 cạnh phải, "Virtual Buttons"): FT6336 báo điểm OUT-OF-BAND Y=1600, X=100/200/300 → HOME/PREV/NEXT (không phải phần cứng thật) |
| Frontlight | Dual cool/warm, GPIO6/7, YX6016N | pha 3 |
| Audio | I2S mic+speaker qua BT chip (UART AT GPIO39/38) | pha 3, stub no-op |
| SD | microSD SDMMC 4-bit GPIO20-25 | thư mục host `./fs_/` |
| Wifi/BT | Wi-Fi on-chip; BT via UART AT | pha 2 |
| Sensors | SC7A20H, PCF8563 RTC, BQ27220 fuel gauge | RTC/gauge trong pha 1, accel pha 3 |
| Khác | Vibration motor, TCA9555 IO expander (0x20), USB MSC | MSC pha 2, haptics stub |
| Kích thước (ảnh #5) | 71 × 122 × 7.8 mm, 4.7" | dùng làm tỷ lệ khung thiết bị trong SDL window |

## 4. Status / Next Actions

- [x] Intent chốt, REA phân tích 4 nguồn (REA, reader-simulator, CloudZao, RickyOS install page)
- [x] Ảnh marketing thiết bị + phím vật lý đã chụp (source cho spec input mapping)
- [x] **Bước 1 (2026-10-28): Clone CloudZao về `vendor/cloudzao/`, đọc `config.h` + `metalio_keys.cc` + `lv_adapter_epdiy.cc` → có GPIO/IO map thật (xem §3 — sửa theo source, bỏ giả định "núm xoay")**. Node v26.12.1 (đủ cho REA), gh token scope bị giới hạn nhưng git HTTPS OK.
- [x] **Spec/plan/tasks + skills + rules + ADR + ref scans** (docs/, .claude/, README, file này) — đã commit+push lần đầu lên `magicxlll/zOnhand` (commit aeb2467)
- [x] **ESP-IDF v6.1 install ở `~/esp/esp-idf`** (Python 3.14 env, target `esp32s31` được hỗ trợ chính thức — B4 hạ cấp). BUILD FW THẬT fail: thiếu managed components `78__uart-uhci`, `espressif__iot_eth` (không có trong `main/idf_component.yml`) → **chờ user decide** (thêm dep + vendor file, hay chấp nhận baseline = harness native).
- [x] **T1 XONG (commits 5a5e2ea, ab73f2a):** harness native `ebook_sim` build + CHẠY thật trên macOS (SDL2, LVGL v9.5, đọc .ebook 1 chapter, >7s clean). Vá 5 chỗ trong `sim/`. Crash SIGSEGV đã fix (root cause: font `.ef` upstream là symlink hỏng → sửa symlink về `vendor/cloudzao/tools/epdfont/MiSans-Light/`). `sim/run.sh` launcher end-to-end.
- [x] **T3 XONG (cbe38a6):** panel **684×1216** (E-Ink4 Plus) thay harness mặc định 480×800.
- [x] **T4 XONG (7e10f53):** input shim — register `lv_sdl_mouse_create`+`lv_sdl_keyboard_create`; phím `←↑a k`=prev, `→↓d j`=next.
- [x] **T5 XONG (71fc056):** virtual-key strip (3 vạch cam HOME/PREV/NEXT trên bezel phải, đúng FT6336 out-of-band) + phím `H`=về trang 1. Display 728×1216 (panel 684 + strip 44).
- [x] **REA `.bin` XONG (09:15):** `docs/feature-list-recommended.md` (đã commit ở 795d606). Phát hiện: .bin = **CrossPoint Reader rebrand (RickyOS), Arduino-ESP32 RC1, KHÔNG cùng codebase** `vendor/cloudzao`. Tính năng riêng (WeRead/KOReader sync, OPDS, gamification, Gomoku, WAV) = Phase 2/3.
- [x] **T2' XONG:** app shell host (Home=lưới thư viện sách + Settings=chọn 字号 qua `.ef` thật) trên `BookSession` thật. Đủ 5 mục: impl `book_reader_show_home`/`settings`, input mode-aware (Home/Settings/Reader), font resolution an toàn + fallback, boot default -> Home, verify end-to-end qua `ebook_test` và `run.sh` (Home hiện sách → Enter mở → đọc J/K/H → Settings đổi font 25→30 render to hơn).
- [x] **T2-full XONG (theo D3, chuỗi shim 1 commit/shim):**
  - [x] `sim/shim/freertos/` — `xTaskCreate`→`std::thread`, `vTaskDelay`, `xEventGroup`, `xQueue`, `xSemaphore`, `esp_timer` (periodic/once via timer thread), `esp_err_to_name`. Đã test PASS 100% qua `test_freertos_shim` (commit `e9464a4`).
  - [x] `sim/shim/nvs/` — `nvs_*` (get/set str, i32, u8, blob, erase, commit, flash_erase) → file-based persistent storage (`$ZONHAND_NVS_DIR` hoặc `~/.zOnhand_sim/nvs/*.dat`). Đã test PASS 100% qua `test_nvs_shim` (commit `b4f7d34`).
  - [x] `sim/shim/wifi_audio_ota_noop/` — `board.StartNetwork()` no-op, audio no-op, OTA skip, `esp_wifi` / `esp_netif` / `esp_random` / `esp_chip_info` / `esp_app_desc` stubs. Đã test PASS 100% qua `test_wifi_audio_ota_shim` (commit `657f76a`).
  - [x] Boot `Application::Start()` thật (verify log boot khớp source `vendor/cloudzao/main/application.cc:1331`) qua `sim/shim/application_boot/test_boot_application` PASS 100% (boot flow, NVS, status bar, MainEventLoop thread scheduling, esp_timer periodic clock ticks). Không sửa 1 dòng code nào trong `vendor/`.
- [x] **Hardware Shim — TCA9555 & Keys (T4):** `sim/shim/hardware/tca9555_keys/` mô phỏng TCA9555 IO expander (I2C 0x20) + BOOT button (GPIO61, IRQ). Link trực tiếp với code gốc `vendor/cloudzao/main/boards/metalio-e-ink4-plus/input/metalio_keys.cc`. Đã verify PASS 100% qua `test_tca9555_keys` (VOL+ P10 down/up edge, VOL- P11 down/up edge, POWER P17 short click, POWER long press >=3000ms triggering shutdown pulse on P13, BOOT GPIO61 interrupt, vibration motor control, pulse train count).
- [x] **Hardware Shim — Battery Gauge & RTC (T6 subset):** `sim/shim/hardware/power_rtc/` mô phỏng BQ27220 Fuel Gauge + PCF8563 RTC + MetalioRtc HAL + MetalioPeriph_StartSystemMonitor. Link trực tiếp với file gốc `vendor/cloudzao/main/boards/metalio-e-ink4-plus/periph/metalio_rtc.cc` và `metalio_sys_mon.cc`. Đã verify PASS 100% qua `test_power_rtc` (default 70% 3850mV discharging, dynamic state update 92% 4180mV charging, host RTC time reading, custom time injection, system monitor background task logging CPU/RAM/Battery metrics).
- [x] **Interactive Emulator Integration (T2' + NVS + HW Keys + Top Status Bar + Standby Mode):** tích hợp toàn diện NVS persistence, physical keys mapping (P/p: Standby toggle, B/b: Home, VOL+/VOL- navigation & page turn), top status bar (pin BQ27220, đồng hồ PCF8563, tên màn hình), và màn hình chờ Standby (wallpaper e-paper, đồng hồ lớn, thông số pin, wake on any key). Đã verify PASS 100% 7 bước qua `test_t2_shell` (commit `ca9fbe9`).
- [ ] **Chờ user review:** danh sách tính năng RickyOS trong `docs/feature-list-recommended.md` (quyết Phase 2 scope).

### Decision (user chốt 2026-10-29, bản đầy đủ)
- **D1 — Cấu trúc VM: "(a) + rail [khuyến nghị]"** — mở rộng harness `ebook-emulator` ĐÃ chạy (demo nhanh), nhưng mọi shim (phím/touch/SD/NVS) viết **theo kiến trúc host-shim** (`sim/shim/`) để sau này đóng thành `sim/` sạch. Không viết lại từ 0.
- **D2 — Build fw thật: "bo qua, chạy VM trước"** — chưa có máy thật để so. Lùi `idf.py build` (thiếu `78__uart-uhci` + `espressif__iot_eth`) sang sau. (Ghi chú: `78/uart-uhci` CÓ trên GitHub `https://github.com/78/uart-uhci`; `espressif/iot_eth` KHÔNG public — cần user chỉ nguồn nếu after decide build thật.)
- **D3 — T2: "T2' rồi T2-full [khuyến nghị]"** — trước: app shell host (Home + Settings trên BookSession thật, ~1 ngày); sau: chuỗi shim FreeRTOS/NVS/wifi → boot `Application::Start()` thật.

## 5. Blockers & Open Questions

- **gh token của `magicxlll` thiếu scope `read:org`** → `gh` CLI không auth được; **git HTTPS OK** (token repo-local `.git/.git-credentials-local`, KHÔNG commit). Backup chạy qua `git push`.
- `RickyOS-Plus-1.2.9-full.bin` là **không phải** fw chính thức phát hành (trang demo ghi "chính thức chưa phát hành") — vật phẩm tham khảo.
- CloudZao fw AI voice (Baiwen) cần Wi-Fi/MQTT/WebSocket — pha 2, stub no-op trong pha 1.
- **Build fw thật (baseline) fail — BO QUA theo D2 (chốt 2026-10-29):** thiếu managed components `78__uart-uhci` + `espressif__iot_eth` (REQUIRES `main/CMakeLists.txt:230` nhưng KHÔNG có trong `main/idf_component.yml`; `managed_components/` ship thiếu). Lùi sang sau. `78/uart-uhci` CÓ trên GitHub; `espressif/iot_eth` KHÔNG public (cần user chỉ nguồn nếu after decide build thật).
- **Chờ user review REA:** danh sách tính năng RickyOS trong `docs/feature-list-recommended.md` (WeRead/KOReader/OPDS/gamification/Gomoku/WAV) → quyết Phase 2 scope.

## 5b. Phát hiện từ source (đúng 100%, dùng cho spec — không được thay bằng đoán)

- **4 nút cảm ứng/physical thật:** BOOT (nút tròn dưới cạnh trái, GPIO61, IRQ BOTHEDGE) · VOL+ (TCA P10) · VOL- (TCA P11) · POWER (TCA P17, idle high). Không có núm xoay/scroll wheel.
- **TCA P13 = pulse output** (PWR_PULSE, 15×100ms) do code phát ra để lệnh tắt máy — không phải nút. Nút "giữa" trên ảnh marketing = P13 = không bấm được.
- **3 Virtual Keys ("Virtual Buttons" = 3 vạch cam cạnh phải)** là FT6336 báo điểm **ngoài vùng màn hình** (Y=1600, X=100/200/300) → vk_home/vk_prev/vk_next, không phải phần cứng.
- **VOL+/VOL- = page_prev/page_next khi đang đọc (đúng ảnh: cạnh phải "dễ cảm nhận" = lướt trang), mới = volume khi ở menu** — semantic kép từ code `lv_adapter_epdiy.cc`.
- BOOT long = shutdown; POWER short = màn hình tắt/đánh thức; VOL+ long = vol max (hoặc page_prev liên tục khi đọc); VOL- long = mute (hoặc page_next).
- **TCA9555 expander** (I2C 0x20, INT=GPIO2) phải được mô phỏng chính xác trong simulator (mỗi phím = bit port).

## 6. File Map (thực tế hiện tại)

- `README.md` — doc chính
- `docs/` — ADRs, intent, spec pha 1 (`spec-phase-1.md`), `feature-list-recommended.md` (REA, chưa commit), **`handoff-2026-10-29.md` (báo cáo chi tiết + prompt phiên sau)**, `ref/` (source-scan ground truth)
- `.claude/skills/` + `.claude/rules/` — skill + rules dự án
- `Project_memory.md` — file này (repo root)
- `vendor/cloudzao/` — source fw (READ-ONLY; HEAD pin `2f55cb1`; gitignored khỏi backup repo)
- `firmware-reference/` — `RickyOS-Plus-1.2.9-full.bin` + output REA
- `sim/ebook-emulator/` — harness native (copy từ `vendor/.../tools/ebook/emulator/`, `lvgl/` = symlink)
- `sim/run.sh` — launcher; `sim/sample/` — .ebook mẫu
