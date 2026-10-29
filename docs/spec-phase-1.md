# zOnhand — Spec / Plan / Tasks: Phase 1 (VM crosspoint-simulator)

> Mục đích: tài liệu **độc lập, self-contained** để (1) agent khác đọc và tiếp tục nếu tôi hết quota; (2) user theo dõi tiến độ. Mọi số liệu đã được verify với source `vendor/cloudzao` (commit được pin trong §9). Đừng "tin" — check §9 trước khi code.

---

## 0. TL;DR — state hiện tại (đọc trước tiên nếu vội)

- [x] Intent + phạm vi Phase 1 đã chốt với user (2026-10-28).
- [x] Source CloudZao đã clone về `vendor/cloudzao` (shallow, 1 commit; pin SHA ở §9).
- [x] **Map GPIO/IO thật đã extract từ code** (BOOT=GPIO61, VOL+/VOL-/POWER=TCA P10/P11/P17, P13=pulse output, virtual keys=FT6336 out-of-band Y=1600/X=100/200/300) — xem `Project_memory.md` §5b. **Không có núm xoay.**
- [x] Kiến trúc chốt: **SDL2 native trên macOS + shim ESP-IDF v6.1 host-sim** (không WASM, không QEMU).
- [ ] **Spec/plan/tasks này** (đang viết — file này).
- [ ] Repo backup: `https://github.com/magicxlll/zOnhand.git` — **chưa init, chưa commit** (token user OK qua HTTPS; `gh` CLI thì không vì token thiếu scope `read:org` — không cần, dùng git trực tiếp).
- [ ] REA phân tích `RickyOS-Plus-1.2.9-full.bin` (song song, không blocking Phase 1).
- [ ] User chưa có máy thật → baseline hành vi = docs + code CloudZao + bằng chứng RickyOS.

**Nếu bạn là agent kế tiếp:** bắt đầu từ mục §2 (Phạm vi Phase 1), rồi §3 (Kiến trúc), rồi chạy task T1 (mục §4) — đó là "đặt nền" cho mọi thứ khác.

---

## 1. Context / Intent (đã chốt với user, giữ nguyên từ ngữ)

- **Từ repo `CloudZao/Metalio-E-INK4-Plus`** (ESP-IDF v6.1, C/C++, LVGL, MIT) xây dựng **trình máy ảo kiểu crosspoint-simulator**: compile **source fw nguyên bản không sửa** native, chạy trong **SDL2 window trên macOS**, shim phần cứng.
- **RickyOS-Plus-1.2.9-full.bin** (đã có trong repo, 16.7MB): binary đóng → **không chạy được trên VM**. Vai trò: "spec sống" — REA phân tích + demo công khai → **đề xuất danh sách tính năng, user review từng mục** → thành spec tính năng + test scenarios verify trên VM.
- **Step 2 (sau khi VM đạt):** fork CloudZao thành nhánh mới, custom tính năng kiểu RickyOS, mỗi tính năng verify bằng test scenario trong VM trước.

### Intent restate (đã confirm)

| Trường | Nội dung |
|---|---|
| Outcome | VM chạy được fw gốc CloudZao, đúng hành vi, cho phép test trước khi flash máy thật |
| User | User (dev đơn, có macOS, chưa có máy thật) |
| Why now | CloudZao là nền tảng cốt lõi; RickyOS là thước đo tiêu chuẩn + nguồn spec tính năng |
| Success | (1) VM boot→hiển thị→phím/touch→đọc sách→settings hành vi trung thực; (2) cùng commit → cùng hành vi với máy thật; (3) file SD thật đặt vào thư mục sim đọc được |
| Constraint | macOS ưu tiên; giữ pipeline build ESP-IDF v6.1 bên cạnh env host-sim |
| Out of scope | Chạy binary .bin đóng; Web/WASM/iOS/Windows; Wi-Fi/cloud (Phase 2); AI voice/BT audio/frontlight (Phase 3) |

---

## 2. Phạm vi Phase 1 (bắt đầu từ đây)

**Thứ tự boot-to-read (đã chốt):**

1. **Boot & state machine** — khởi động từ image, vào màn hình chính
2. **Hiển thị EPD** (ED047TC2, 684×1216 portrait) — full + partial refresh, dither, không scroll
3. **Phím vật lý + touch FT6336** — map GPIO thật (xem §3.3)
4. **Đọc sách** — lưới bìa 3×3 → chi tiết → nội dung; `.epub/.txt` (+ pipeline `.ebook`)
5. **Settings tối thiểu** — refresh mode, volume keys page
6. **NVS + RTC + battery gauge** — đủ để fw không lệch trạng thái

**Tiêu chí "thông" Phase 1 (đo được):**
- Chạy **đúng source build** trong SDL2 window; hiển thị trung thực e-paper; thao tác phím/touch điều khiển được toàn bộ luồng trên
- Cùng commit source → `idf.py build` ra `.bin` flash máy thật, **hành vi các luồng trùng** (so sánh khi có máy thật; hiện baseline = docs + RickyOS bằng chứng)
- File SD (books/dictionary) trên máy thật đặt vào thư mục sim đọc được

---

## 3. Kiến trúc

### 3.1 Tổng quan

```
┌────────────────────────────────────────────────────────────┐
│  CloudZao firmware source (không sửa)                     │
│  main/application.cc, boards/metalio-e-ink4-plus/, ...    │
└────────────────────────────────────────────────────────────┘
                        │ (link-time)
                        ▼
┌────────────────────────────────────────────────────────────┐
│  Shim ESP-IDF host layer (viết mới)                       │
│  esp_log, esp_timer, nvs, driver/gpio, i2c_master,        │
│  sdmmc, freertos (→pthread/cond), lvgl (→SDL2 renderer)  │
└────────────────────────────────────────────────────────────┘
                        │ (SDL2 window, input event)
                        ▼
┌────────────────────────────────────────────────────────────┐
│  SDL2 app shell (viết mới)                                │
│  - Window 684×1216 (portrait), e-paper palette (16 xám)  │
│  - Map key/touch host → GPIO/TCA/FT6336 shim state        │
│  - SD card → ./fs_/ thư mục host; NVS → ./fs_/.nvs/     │
└────────────────────────────────────────────────────────────┘
```

**Giống crosspoint-simulator:** firmware compile native, chạy unmodified, render SDL2. **Khác:** CrossPoint dùng PlatformIO/Arduino; CloudZao dùng ESP-IDF v6.1 (CMake) → ta viết **host-sim layer cho ESP-IDF** (nhiều việc hơn, nhưng giữ pipeline build gốc).

### 3.2 Module shim ESP-IDF host (đầy đủ, theo thứ tự cần viết)

| # | API cần shim | Dùng ở đâu trong fw | Ghi chú |
|---|---|---|---|
| S1 | `freertos.h` (Task, Queue, Mutex, Timer) | khắp nơi | mapping → pthread + std::mutex + std::condition_variable; stack/task → stack size bỏ qua |
| S2 | `esp_log.h` | khắp nơi | → stdout (colorized, tag-prefix), có `ESP_LOG_LEVEL` filter |
| S3 | `esp_timer.h` | key debounce, tick loop | → `std::chrono::steady_clock` + thread pump |
| S4 | `driver/gpio.h` (`gpio_config`, `gpio_set_level`, `gpio_get_level`, ISR) | BOOT (GPIO61), TCA INT (GPIO2), FL (GPIO6/7), ANA_SW (GPIO36) | GPIO state giữ trong `SimGpioRegistry`; BOOT=host keyboard key F9 (đề xuất); ISR callback → event queue |
| S5 | `i2c_master.h` (TCA9555 @0x20, FT6336 @0x38, PCF8563 @0x51, BQ27220 @0x55, SC7A20H @0x19) | key (P10/P11/P13/P17), touch, RTC, gauge | **I2C simulator** = virtual bus + device registry; host keyboard → TCA port1 bit; host mouse/touch → FT6336; RTC host `time()`; gauge = slider UI nhỏ (optional Phase 1, default 70%) |
| S6 | `sdmmc` + `fatfs` (host fs) | đọc `.epub/.txt/.ebook`, dictionary, wallpaper | map → `./fs_/` (thư mục host); file I/O thật, không RAM disk |
| S7 | `nvs_flash.h` / `nvs.h` | persist config (last book, volume, refresh mode) | map → `./fs_/.nvs/` (JSON/protobuf đơn giản) |
| S8 | `lvgl` (renderer) | toàn bộ UI | **khó nhất**. lvgl build native (đã làm được trong nhiều project). Custom `lv_display` driver → SDL2 texture 684×1216, palette 16-gray e-paper, partial refresh qua `lv_refr_now` + dirty rect |
| S9 | `esp_wifi.h` / `esp_http_client` / `mqtt` | (Phase 2) | Phase 1: **stub no-op**, log "wifi disabled in sim" |
| S10 | `i2s.h` (audio) | (Phase 3) | stub no-op |
| S11 | `esp_system.h` (reset, reboot, deep sleep) | boot/shutdown | `esp_deep_sleep` → exit sim (in Phase 1, shutdown pulse = exit) |
| S12 | `esp_app_main` / `main()` entry | boot | wrapper gọi `app_main()` của fw |

**Thứ tự build:** S1→S4 (nền), rồi S2/S3 (log/timer), S5 (I2C/keys/touch), S6/S7 (SD/NVS), **S8 (LVGL→SDL2 = blocker chính)**, S9/S10 stub.

### 3.3 Input Mapping Ground-Truth (đã verify từ `vendor/cloudzao/main/boards/metalio-e-ink4-plus/config.h` + `metalio_keys.cc` + `lv_adapter_epdiy.cc`)

**KHÔNG có núm xoay.** 4 nút physical/capacitive + 3 virtual keys (out-of-band touch):

| Nút | Vị trí thật | Map firmware | Shim host (đề xuất) |
|---|---|---|---|
| BOOT (nút tròn cam, dưới cạnh trái) | GPIO61, IRQ BOTHEDGE, pull-up | click = home/ctx; long (3s) = shutdown pulse; double = (xem code) | Key `B` (click), giữ ≥3s = shutdown |
| VOL+ (cạnh phải, "1" trên ảnh Dual Modes) | TCA P10 (idle high, down=low) | Khi đang đọc: page **prev** (click) / page prev repeat (long); khi menu: volume +10 (click) / vol=100 (long) | Key `=` (hoặc `]`) |
| VOL- (cạnh phải, "2") | TCA P11 | Khi đang đọc: page **next** (click) / repeat (long); khi menu: vol −10 / mute | Key `-` (hoặc `[`) |
| POWER (cạnh phải, "3") | TCA P17 | click = toggle standby; long (3s) = shutdown pulse (P13, 15×100ms) | Key `P` |
| (P13 = shutdown pulse OUTPUT, không phải nút — code phát ra để tắt máy; ảnh marketing "núm giữa" = P13, **không bấm được**) | TCA P13, output | `MetalioKeys_TakeShutdownPulse()` | sim: không cần map, là output-only |
| Virtual "1" (HOME, cạnh phải, 1/3 trên) | FT6336 out-of-band, Y=1600±30, X=100 | `VkKey_Dispatch("vk_home")` | Host: click region trên frame (vẽ ra ngoài vùng e-paper), hoặc key `H` |
| Virtual "2" (PREV/UP, cạnh phải, 2/3) | Y=1600, X=200 | `vk_prev` = back / page up (repeat long) | key `K` (đề xuất, lên) |
| Virtual "3" (NEXT/DOWN, cạnh phải, 3/3) | Y=1600, X=300 | `vk_next` = forward / page down (repeat long) | key `J` (đề xuất, xuống) |
| Touch (FT6336, I2C 0x38, INT=GPIO5, RST=TCA P12) | in-band (Y<1600) | tap/swipe vùng màn hình | Host mouse click/drag trên e-paper region |

**Ghi chú quan trọng (đúng với ảnh marketing, đừng sửa):**
- Ảnh #4 gọi cụm (1) cạnh phải là **"Physical Buttons"** (3 nút vật lý = VOL+/VOL-/POWER) và (2) nút tròn cam cạnh trái là **nút cảm ứng** ("Easy to Feel, So You Can Keep Your Eyes on the Screen") — user đã xác nhận "nút tròn là nút cảm ứng, không phải núm." → BOOT.
- "3. Virtual Buttons — Tap on the Screen, Right at Your Fingertips" = 3 vạch cam (1/2/3) trên cùng cạnh phải **báo qua FT6336 out-of-band** (Y=1600), **không phải phần cứng.**
- Kích thước máy: **71 × 122 × 7.8 mm** (ảnh #5) → tỷ lệ khung thiết bị trong SDL window = 1:1 px (1px = 1mm/71, hoặc scale 1.0).
- VOL+/VOL- **mặc định là volume trong menu, là page prev/next khi đang đọc** (xem code `lv_adapter_epdiy.cc:566-600`). Đừng "tương tác" thành volume đơn thuần.

### 3.4 Display / E-paper Rendering

- ED047TC2: native 1216×684, UI portrait 684×1216 (`config.h`). `DISPLAY_CONTENT_*_INSET = 2` (trừ 2px mỗi cạnh cho nội dung; wallpaper/standby dùng full panel).
- Refresh: **full** (bật tắt toàn màn, có ghost) + **partial** (chỉ redraw dirty region, nhanh). `render.c`/`highlevel.c` trong `main/boards/.../display/` điều khiển. Phase 1 cần cả hai; dither bật (xem `docs/dither.md` nếu có).
- Sim: palette 16 gray (bump standard e-paper), không timing thật, không LUT — "framebuffer only" (giới hạn chấp nhận, đúng crosspoint). 
- SDL2: 1 window kích thước 684×1216 (có thể scale 2x cho dễ nhìn), SDL_CreateTexture + SDL_UpdateTexture theo dirty rect (partial) hoặc toàn bộ (full).

### 3.5 SD / NVS / RTC / Gauge

- SD → `./fs_/` (host thư mục). `.epub` reader đọc trực tiếp; `.ebook` (format custom của fw, xem `tools/ebook/`) cần pipeline convert trước (hoặc đọc raw).
- NVS → `./fs_/.nvs/` (JSON, keyed by namespace). `nvs_open`/`nvs_set_*`/`nvs_commit` map tương ứng.
- RTC (PCF8563 @0x51) → host `std::chrono` (không persist, ok cho Phase 1; nếu cần giữ qua restart sim, ghi file).
- Battery gauge (BQ27220 @0x55) → giá trị mặc định 70% (trùng ảnh marketing "170%"?? không, đúng là battery icon ~70%), hoặc đọc từ file `./fs_/.sim_battery`. Phase 1 có thể hardcode 70%, đủ cho UI hiển thị.

### 3.6 Build Pipeline (giữ ESP-IDF gốc + thêm env sim)

```
cloudzao/                          # source, KHÔNG sửa
├─ CMakeLists.txt                   # (gốc) ESP-IDF project
├─ sim/                             # (MỚI) — host-sim layer
│  ├─ CMakeLists.txt                # build target `sim` link fw source + shim
│  ├─ shim/                         # S1..S11 (mỗi file = 1 API shim)
│  ├─ sdl/                          # app shell + renderer + input
│  └─ build.sh                      # cmake -DSIM=ON ...
└─ (không thêm [env:simulator] kiểu PlatformIO — khác CrossPoint)
```

**Cạm bẫy (ghi lại để không lặp):**
- ESP-IDF v6.1 cho target `esp32s31` (không phải `esp32s3`). Kiểm tra `sdkconfig.defaults.esp32s31` — build sim phải dùng cùng `sdkconfig` tuned, không tự tạo.
- `patches/esp-idf/` trong repo (CloudZao) — có thể cần apply khi build host (xem mục §6 Blocker #3).
- PSRAM Octal @200MHz — host không có, nhưng fw dùng `heap_caps_*` API → shim malloc (không cần PSRAM thật, chỉ cần flag).
- `s31_sleep/` (MPLL/PSRAM sleep compat) — **không** chạy trên host; stub `s31_sleep_init` = no-op, log.

---

## 4. Tasks (có tracking, dependency rõ, cho agent khác tiếp được)

> Nguyên tắc: mỗi task có **(a) what, (b) files sẽ chạm, (c) acceptance criteria, (d) ước lượng.** Task T1 là "nền," không chạy được task khác trước nó. **Không code trước khi T1 xong.**

### T1 — Đặt nền build host (EST: M) — BLOCKER
- **What:** Build được 1 skeleton `sim` (cmake + SDL2 + pthread) link được `main/application.cc` (fw) nhưng chưa chạy logic; `idf.py` vẫn build được bản flash thật (không bị phá pipeline gốc).
- **Files:** `sim/CMakeLists.txt`, `sim/shim/freertos_stub.h/.cc`, `sim/shim/esp_log_stub.h/.cc`, `sim/sdl/main.cc`, `sim/build.sh`.
- **Acceptance:**
  - `cd sim && ./build.sh` → ra binary `build/sim` chạy được, in log "sim boot, app_main not yet wired."
  - `cd vendor/cloudzao && idf.py build` (hoặc `idf.py -DSDKCONFIG=... build`) vẫn pass, ra `.bin` như trước.
- **Ghi chú:** đây là task "rủi ro cao nhất" — ESP-IDF v6.1 + target s31 + cmake host là thứ chưa ai làm cho CloudZao. **Đừng nhảy qua task khác trước khi T1 xong.**

### T2 — Wiring app_main + boot state machine (EST: M)
- **What:** gọi `app_main()` thật của fw trong sim; boot → standby/main screen.
- **Files:** sửa `sim/sdl/main.cc` (wiring), có thể stub thêm `esp_system.h` (reset/reboot).
- **Acceptance:** mở sim → thấy màn hình boot/logo (nếu fw có) rồi vào main screen (lưới 3×3) trong <5s. Không crash.

### T3 — EPD display shim (SDL2 renderer, full+partial) (EST: XL, RỦI RO CAO)
- **What:** lvgl custom driver → SDL2 texture 684×1216, palette 16-gray e-paper, dirty-rect partial refresh.
- **Files:** `sim/shim/lvgl_sdl_display.cc`, `sim/shim/epd_shim.cc` (bọc `epd_highlevel.h` API), sửa `render.c`/`highlevel.c` call sites **nếu** cần (cố gắng không sửa fw).
- **Acceptance:** main screen render đúng layout (lưới 3×3: 阅读/壁纸/百问AI/每日清单/传输/设置), màu e-paper (không RGB). Click icon → screen change (chưa cần logic, chỉ visual). Full refresh = toàn màn blink 1 lần (giả lập), partial = chỉ vùng đổi.

### T4 — Key/TCA shim (4 nút physical + host keyboard) (EST: M)
- **What:** TCA9555 P10/P11/P17 + GPIO61 → host keyboard mapping (§3.3).
- **Files:** `sim/shim/i2c_tca_shim.cc`, `sim/sdl/input.cc`, sửa `epd_board_metalio_eink4_plus_ioexp_read_port` (nếu là API thật, shim tầng trên).
- **Acceptance:** B=`BOOT click`, `=`/`-`=VOL+/VOL-, `P`=POWER, giữ ≥3s = long. In log đúng `metalio_keys.cc` (khớp `MetalioKeys_Init` output thật).

### T5 — Touch FT6336 shim (out-of-band virtual keys + in-band) (EST: M)
- **What:** host mouse/touch → FT6336 coords (in-band Y<1600), out-of-band Y=1600/X∈{100,200,300} → vk_home/vk_prev/vk_next.
- **Files:** `sim/shim/ft6336_shim.cc`, `sim/sdl/input.cc` ( thêm vùng "virtual key strip" bên cạnh e-paper trong SDL window, hoặc dùng phím J/K/H).
- **Acceptance:** click strip "1/2/3" → dispatch đúng `vk_home`/`vk_prev`/`vk_next` (log khớp). Swipe book → page turn (in-band).

### T6 — SD/NVS shim (EST: S–M)
- **What:** `./fs_/` host thư mục; NVS → JSON.
- **Files:** `sim/shim/sdmmc_host.cc`, `sim/shim/nvs_host.cc`, `sim/shim/rtc_host.cc`.
- **Acceptance:** đặt 1 `.txt`/`.epub` thật vào `./fs_/Books/` → main screen đọc được (list ra). Restart sim → config (last book) vẫn giữ (NVS).

### T7 — Book reader flow (3×3 grid → detail → content) (EST: M–L)
- **What:** wire logic đọc (chưa cần AI); `.txt` trước, `.epub` sau; `.ebook` convert bằng `tools/ebook`.
- **Files:** sửa `main/display/screen/` (nếu fw không "quá hardware" để chạy, chỉ cần không đụng — ưu tiên không sửa).
- **Acceptance:** mở 1 `.txt` → nội dung render, page prev/next bằng VOL+/VOL- (chính xác semantic kép §3.3), back bằng vk_prev.

### T8 — Settings + refresh mode + volume (EST: S)
- **Acceptance:** vào Settings (icon ⚙) → đổi refresh mode (full↔partial) thấy khác biệt; vol keys (menu context) đúng ±10/mute/max.

### T9 — Test scenario (từ spec RickyOS, song song REA) (EST: M, phụ thuộc REA output)
- **What:** danh sách tính năng REA đề xuất (xem §7) → viết test scenario cho **subset Phase 1** (boot, keys, touch, reading, settings, NVS/RTC/gauge). Mỗi scenario = script (key seq + assert screen state).
- **Files:** `sim/tests/*.py` (hoặc C++).
- **Acceptance:** từng scenario pass trên sim, log rõ.

### T10 — Backup + doc (EST: S, chạy song song)
- **What:** commit repo lên `magicxlll/zOnhand` (git HTTPS, token OK). Mỗi task xong = 1 commit + cập nhật `Project_memory.md` + file này (đánh dấu `[x]`).
- **Acceptance:** `git log --oneline` tăng dần, `git push` thành công.

### T11–T13 (Phase 2, không bắt đầu đến khi Phase 1 "thông")
- T11: Wi-Fi / host curl shim, WebServer 127.0.0.1, OTA (no-op hoặc host-side).
- T12: USB MSC (host mount?), AP transfer.
- T13: frontlight, BT audio stub, checklist cloud sync.

---

## 5. Acceptance Criteria Phase 1 (tổng)

1. `./sim/build.sh` ra binary chạy được trên macOS (arm64 + x86_64).
2. Boot → main grid render đúng (visual so với ảnh marketing, 684×1216 portrait, e-paper palette).
3. 4 physical keys + 3 virtual keys + in-band touch đều dispatch đúng (log khớp `metalio_keys.cc`).
4. Đọc 1 `.txt`/`.epub` thật trong `./fs_/Books/` → page turn bằng VOL+/VOL- (semantics §3.3).
5. Restart sim → last book + volume + refresh mode giữ (NVS).
6. `idf.py build` (bản flash thật) vẫn pass trên cùng commit (không phá pipeline gốc).
7. Mỗi test scenario §4-T9 pass.

---

## 6. Blockers / Known Risks (đọc kỹ nếu tiếp tục)

| # | Rủi ro | Mitigation |
|---|---|---|
| B1 | ESP-IDF v6.1 host-sim chưa có precedent cho CloudZao (crosspoint dùng PlatformIO/Arduino) | T1 là task "go/no-go": build được skeleton mới chạy tiếp; không thì escalate (đề xuất: cân nhắc fork sang PlatformIO như crosspoint, **nhưng phải hỏi user trước** vì nó đổi pipeline gốc). |
| B2 | LVGL e-paper driver (dither, partial refresh) phức tạp, dễ sai màu | T3 có visual regression test: render 1 screen, so ảnh PNG với baseline (chụp từ máy thật khi có, trước đó so với screenshot fw). |
| B3 | `patches/esp-idf/` trong CloudZao (MPLL/s31_sleep compat) | Stub `s31_sleep*` no-op trên host; chỉ apply patches khi build flash thật. |
| B4 | `esp32s31` vs `esp32s3` — tên target lạ, SDK có thể thiếu | Kiểm tra `sdkconfig.defaults.esp32s31` + `idf.py --target esp32s31` trước khi build sim. |
| B5 | CloudZao cloud endpoints "empty" trong build open-source (README §13.1) | Phase 1 không cần cloud; stub no-op. |
| B6 | `gh` CLI không authenticate được (token thiếu `read:org`) | Dùng `git` HTTPS trực tiếp (đã OK). Không cần `gh`. |
| B7 | RickyOS .bin là "đáng tin" nhưng không source → REA chỉ trích spec, không phải copy code | Đặt đúng kỳ vọng (user đã confirm). |

---

## 7. REA Workflow (song song, không blocking Phase 1)

1. `npx rea-agents setup` (Node v26.12.1 đã OK) → REA MCP server + skill.
2. REA analyze `RickyOS-Plus-1.2.9-full.bin` (regions, strings, native handoffs, symbols nếu có).
3. REA analyze demo page `https://rickyaistudio.com/install.html` (feature list công khai, đã fetch trước — xem `docs/ref/rickyaistudio-install.html.md` nếu có).
4. Output → `docs/rickycap-spec.md`: **danh sách tính năng (feature → evidence in .bin/demo → đề xuất impl trong fork)** — user review từng mục, chọn subset Phase 2/3.
5. Không phải copy binary; chỉ trích **spec + test scenario** cho sim verify.

**Evidence đã có từ scan (2026-10-28, `WebFetch` install.html):** RickyOS 1.2 target "MindReset Read Pico (RDP-G01-W) / Metalio E-Ink4 Plus / EEGO A4"; features: "小狗伴读模式" (dog-companion reading mode), auto-detect factory vs flashed, "SD card không bị đụng," local-only (không upload), op log export. Download button trong trang là placeholder (`#`) — file .bin thật user đã có.

---

## 8. Repo / Vận hành (skill + rules + docs + memory + backup)

### 8.1 Layout đề xuất (đây là file này + các file khác)

```
zOnhand/
├─ Project_memory.md                 # (đã có) — state ngắn, cập nhật mỗi debug
├─ README.md                         # (TODO) — entry point, TL;DR + link
├─ .claude/
│  ├─ skills/                        # (TODO) — skills dự án (phong cách agent-skills)
│  │  ├─ simulator-build.md          #   build/run sim (T1–T8)
│  │  ├─ rickycap-research.md        #   REA workflow (mục 7)
│  │  └─ firmware-verify.md          #   chạy test scenario (T9)
│  └─ rules/
│     └─ project-rules.md            # (TODO) — rules: "không sửa fw nếu không cần," "cập nhật memory sau mỗi debug," "commit+push sau mỗi task"
├─ docs/
│  ├─ spec-phase-1.md               # (file này, nếu tách) hoặc giữ ở đây
│  ├─ plan-phase-1.md               # (mục 4, tách ra nếu dài)
│  ├─ adr/0001-sdl2-native-sim.md   # (TODO) — quyết định kiến trúc (bằng chứng crosspoint-simulator + QEMU limit + ESP-IDF v6.1 vs PlatformIO)
│  ├─ rickycap-spec.md               # (TODO, sau REA)
│  └─ ref/                           # evidence scans (install.html, reader-simulator README, crosspoint-simulator README)
├─ vendor/cloudzao/                  # (đã clone)
├─ sim/                              # (T1+)
├─ firmware-reference/               # RickyOS .bin + output REA
└─ .git/                              # (init, remote=magicxlll/zOnhand)
```

### 8.2 Git / backup

- `git init -b main` (đã init, empty).
- `git remote add origin https://github.com/magicxlll/zOnhand.git` (đã set).
- Credential: token `magicxlll` (ghp_...) đã verify hoạt động qua HTTPS. Lưu ở `.git/.git-credentials-local` (outside working tree, không commit). **Đừng commit token.**
- Mỗi task xong: 1 commit (msg theo conventional: `feat(sim): T3 EPD SDL2 renderer`), `git push origin main`.
- `gh` CLI không authenticate được (token thiếu `read:org`) — **không cần**, dùng `git` trực tiếp.

### 8.3 Skill (mục 8 của user)

Mỗi skill = 1 file markdown trong `.claude/skills/`, phong cách `agent-skills` (title + overview + when to use + steps). 3 skills tối thiểu:
1. `simulator-build` — "build/run sim, debug common T1–T8 failure"
2. `rickycap-research` — "chạy REA trên .bin, viết rickycap-spec"
3. `firmware-verify` — "chạy test scenario, báo pass/fail theo §5"

### 8.4 Project_memory.md (đã có)

Cập nhật **bắt buộc** sau mỗi debug/task (ghi: gì đã chạy, gì đã fix, gì còn dở, next action). Đây là "mắt cửa" cho agent kế tiếp.

---

## 9. Pin / Ground-truth Sources (đừng "tin" — check)

| Source | Vị trí | Ghi chú |
|---|---|---|
| CloudZao source | `vendor/cloudzao/` (shallow clone) | **PIN HEAD = `2f55cb1234baae4968982feb23e7a9001dac16f0`** (verify lại `git rev-parse HEAD` sau mỗi pull) |
| ESP-IDF | tag `v6.1` (tồn tại, verify 2026-10-28) — đang install ở `~/esp/esp-idf` | target `esp32s31` (RISC-V, `CONFIG_IDF_TARGET_ARCH_RISCV=y` trong sdkconfig). Rủi ro B4: toolchain RISC-V cho s31 phải install được — check khi IDF xong. |
| Board config | `vendor/cloudzao/main/boards/metalio-e-ink4-plus/config.h` | GPIO/IO map, TCA port, touch offset |
| Key driver | `vendor/cloudzao/main/boards/metalio-e-ink4-plus/input/metalio_keys.cc` | debounce, power click latch, shutdown pulse |
| Dispatch | `vendor/cloudzao/main/display/lv_adapter_epdiy.cc` (520–660) | VOL+/VOL- semantics (reading vs menu) |
| Virtual key | `vendor/cloudzao/main/display/lv_adapter_epdiy_touch.cc:45-47`, `vk_key_handler.h` | vk_home/vk_prev/vk_next |
| Partition table | `vendor/cloudzao/partitions.csv` | NVS/OTA/SPiffs offsets (S7 dùng) |
| RickyOS .bin | `firmware-reference/RickyOS-Plus-1.2.9-full.bin` (16.7MB, đã copy) | target REA (mục 7) |
| RickyOS demo page | `https://rickyaistudio.com/install.html` (đã WebFetch 2026-10-28) | feature list công khai |
| User images | (4 ảnh trong conversation) | Dual Interaction Modes, 71×122×7.8mm | 

**ESP-IDF:** v6.1 (khớp `sdkconfig.defaults.esp32s31`). Kiểm tra `idf.py --version` trước khi build.

---

## 10b. Cập nhật phát hiện (2026-10-28, sau khi clone + đọc source)

### Phát hiện #1 — CloudZao ĐÃ CÓ sẵn harness native-LVGL (giảm mạnh B1)

`vendor/cloudzao/tools/ebook/emulator/` là harness build **LVGL native (SDL) + WASM** cho module `main/reader` (nucleus đọc sách), dùng:
- `src/esp_compat/` (8 header shim ESP-IDF: esp_log, esp_heap_caps, esp_timer, esp_err, esp_cpu, esp_partition, ff.h, sd_paths.h).
- Guard compile-time `EBOOK_EMULATOR` (đúng 3 file trong `main/reader/`: book_session.cc, epub_document.cc, ebook_document.cc) — khi define, fw loại include FreeRTOS/esp thật, chạy native.
- Panel target là **E-Ink 4 (480×800) chứ KHÔNG phải Plus (684×1216)** — tức harness này chạy được module reader trên host, nhưng chưa phải "VM Plus".
- LVGL vendored: `tools/ebook/emulator/lvgl/` (52MB, không có demos/examples → phải tắt `LV_BUILD_DEMOS/EXAMPLES` khi build).

**Hệ quả:**
- **T1 (đặt nền build host) KHÔNG cần viết từ đầu** — dùng harness có sẵn làm proof-of-concept, rồi mở rộng (thêm shim + panel 684×1216 + key/touch) thành "VM Plus" thật sự. Rủi ro B1 giảm từ "viết ESP-IDF host-sim layer từ 0" thành "mở rộng harness có sẵn + bổ sung shim key/touch/I2C/SD/NVS".
- **T7 (book reader flow)** gần như có sẵn (module reader đã build native được, đúng panel Plus cần đổi size 684×1216).

### Phát hiện #2 — Build fw thật (baseline) fail vì thiếu managed components

`idf.py build` (ESP-IDF v6.1, target `esp32s31`) fail:
- `78__uart-uhci`, `espressif__iot_eth` (và có thể `esp_driver_cam`/`esp_driver_i2c`) **không có trong `main/idf_component.yml`** nhưng được `REQUIRES` trong `main/CMakeLists.txt:230`.
- Có vẻ repo ship `managed_components/` (19 thành viên, nhưng thiếu `78__uart-uhci` + `espressif__iot_eth`) — clone không fetch đủ, hoặc repo upstream thiếu lockfile.
- **Không blocking T1** (harness native dùng LVGL vendored, không cần IDF). Nhưng acceptance "idf.py build vẫn pass" (T1/T-baseline) cần: (a) thêm `78/uart-uhci` + `espressif/iot-eth` vào `main/idf_component.yml` (SỬA FILE VENDOR — cần user approve), hoặc (b) `idf.py --install-shdeps`/re-fetch component manager.
- ⚠️ Chờ user approve việc sửa `vendor/cloudzao/main/idf_component.yml` (rule §1.1: không sửa vendor).

### Phát hiện #3 — `App_Main` boot phụ thuộc network (điểm chết T2)

`Application::Start()` (main/application.cc:1331): `chat_cloud::InitializeFromNvs` → `board.StartNetwork()` → `CheckNewVersion`/`CheckChatActivation`. Trong build open-source cloud endpoints **trống** (`api::HasDefaultOtaUrl()==false`), nên network/activation sẽ skip (xem `StartXiaozhiVoice` đã guard). T2 phải verify: boot có **chặn** ở `board.StartNetwork()` (mất mạng → treo?) không — cần đọc `board.StartNetwork` + stub wifi. **Ghi chú: đây là rủi ro T2, chưa verify.**

### Phát hiện #4 — `ebook_sim` CHẠY được trên macOS (T1 de-risk XONG) + crash đã fix

Harness `sim/ebook-emulator/` (copy từ `vendor/.../tools/ebook/emulator/`, symlink lvgl) **build + chạy thật**: SDL2 window 684×1216 mở, LVGL init, đọc `.ebook` (1 chapter), chạy ổn >7s không crash. Vá trong `sim/` (không đụng vendor):

1. `lv_conf.h`: `LV_USE_SDL=1`, `LV_USE_X11=0`, `LV_USE_EVDEV=0`, `LV_USE_THORVG_INTERNAL=0`, `LV_USE_LOTTIE=0`, `LV_USE_VECTOR_GRAPHIC=0`, `LV_BUILD_DEMOS/EXAMPLES=0`.
2. `src/emulator/image_util_emulator.cc`: `DecodeImageToL8`/`DecodeImageFileToL8` lên 7-arg (khớp header vendor mới); thêm `WriteFileAtomic` + `DecodeZipEntryImageToL8` (host: `ZipReader::ReadEntry` → buffered decode).
3. `CMakeLists.txt`: thêm `-L$(SDL2_LIBRARY_DIRS)` (Homebrew pkg-config thiếu libdir → `-lSDL2main` không resolve).
4. `src/main_ebook.c`: default font path fallback (không cần env).
5. **Crash SIGSEGV đã fix (root cause):** `emulator/assets/fonts/misans_25_2.ef` là **symlink hỏng upstream** (88-byte text chứa path tuyệt đối của tác giả gốc) → `epdfont_open` đọc sai magic → NULL → crash `lv_font_get_line_height(NULL)`. Sửa bằng symlink đúng về `vendor/cloudzao/tools/epdfont/MiSans-Light/misans_25_2.ef` (font thật, magic `EPDFONT`).
6. `sim/run.sh`: launcher end-to-end (build + gen sample `.ebook` + chạy, không cần env).

**Run:** `cd sim && ./run.sh`. **Panel hiện tại: 480×800 (E-Ink4)** — T3 sẽ retarget 684×1216 (Plus).

## 11. Next Actions (cập nhật 2026-10-29)

- [x] **T1 (revise):** CHẠY được `ebook_sim` trên macOS. ✅ XONG.
- [x] **T3:** retarget panel 684×1216 (commit cbe38a6). ✅ XONG.
- [x] **T4:** input shim — register mouse + keyboard indev (LVGL v9.5 `lv_sdl_mouse_create`/`lv_sdl_keyboard_create`), map LEFT/UP/PREV/a → PrevPage, RIGHT/DOWN/NEXT/d → NextPage (commit 7e10f53). ✅ XONG (đã chạy thật, không crash).
- [ ] **Decision 1 (user chốt 2026-10-29):** "Bo qua, chạy VM trước" — build fw thật (thiếu `78__uart-uhci` + `espressif__iot_eth`) lùi sang sau. Nếu sau này build thật: `78/uart-uhci` có trên GitHub (https://github.com/78/uart-uhci); `espressif/iot_eth` KHÔNG có GitHub công khai (404) → cần user chỉ nguồn. Ghi chú: repo upstream gitignore `managed_components/` + không ship `dependencies.lock` khai báo đủ → clone sạch thiếu 2 component này.
- [ ] **Decision 2 (user chốt 2026-10-29):** "Cấu trúc VM = (a)+rail [khuyến nghị]" — mở rộng harness ebook-emulator (demo nhanh), các shim viết theo kiến trúc host-shim để sau này đóng thành `sim/` sạch. T4 đúng là bước đầu của "rail" (shim input module, không hack code reader).
- [x] **T5 XONG (commit 71fc056):** virtual-key strip — 3 vạch HOME/PREV/NEXT (bezel 44px bên phải, đúng vị trí out-of-band FT6336) + phím H/J/K. Reader giữ đúng 684×1216, display 728×1216 (panel + strip). `run.sh` chạy end-to-end clean.
- [x] **T2' XONG:** host app shell (Home thư viện sách + Settings chọn 字号 20-40pt) trên BookSession thật. Navigation: Enter mở sách, J/K lật trang, H về trang 0 / Home, Settings đổi font 25->30 render to hơn.
- [x] **T2-full XONG (theo D3, chuỗi shim 1 commit/shim):** chuỗi shim FreeRTOS (`xTaskCreate`→`std::thread`, `esp_timer`), NVS (file-based persistent), wifi/audio/OTA no-op để boot `Application::Start()` (vendor/cloudzao/main/application.cc:1331) chạy qua 100% với log boot chuẩn xác, không sửa 1 dòng code trong vendor/.
- [x] **T4 (Hardware Shim — TCA9555 & Keys):** mô phỏng hoàn chỉnh TCA9555 IO expander (I2C 0x20) + BOOT button (GPIO61, IRQ BOTHEDGE) trong `sim/shim/hardware/tca9555_keys/`, link trực tiếp với file gốc `vendor/cloudzao/main/boards/metalio-e-ink4-plus/input/metalio_keys.cc`. Đã verify PASS 100% qua `test_tca9555_keys` (VOL+ P10 down/up edge, VOL- P11 down/up edge, POWER P17 short click, POWER long press >=3000ms triggering shutdown pulse on P13, BOOT GPIO61 interrupt, vibration motor control, pulse train count).
- [x] **T6 (Hardware Shim — Fuel Gauge & RTC):** mô phỏng BQ27220 Fuel Gauge + PCF8563 RTC trong `sim/shim/hardware/power_rtc/`, link trực tiếp với `vendor/cloudzao/main/boards/metalio-e-ink4-plus/periph/metalio_rtc.cc` và `metalio_sys_mon.cc`. Đã verify PASS 100% qua `test_power_rtc` (default 70% 3850mV discharging, dynamic state update 92% 4180mV charging, host RTC time reading, custom time injection, system monitor background task logging CPU/RAM/Battery metrics).
- [x] **REA phân tích `RickyOS-Plus-1.2.9-full.bin` (mục 7):** đã scan xong, `docs/feature-list-recommended.md` đã commit (795d606), chờ user review tính năng Phase 2.

---

## Phụ lục A — Cross-Reference với `Project_memory.md`

- `Project_memory.md` = state ngắn, cập nhật liên tục (đọc trước).
- File này = **spec + plan + tasks** (đọc khi bắt đầu làm).
- `.claude/rules/project-rules.md` (TBD) = rules hành xử.
- `.claude/skills/*` (TBD) = workflow cụ thể.
