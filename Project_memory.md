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
- [ ] **Spec/plan/tasks pha 1** — đủ context, viết tiếp theo user request (chi tiết, có tracking, cho agent khác đọc tiếp được)
- [ ] REA phân tích `RickyOS-Plus-1.2.9-full.bin` → danh sách tính năng (đề xuất, user review)
- [ ] Vận hành dự án: skill + rules + docs + file này + backup lên `magicxlll/zOnhand` (git HTTPS đã OK; gh CLI chưa)

## 5b. Phát hiện từ source (đúng 100%, dùng cho spec — không được thay bằng đoán)

- **4 nút cảm ứng/physical thật:** BOOT (nút tròn dưới cạnh trái, GPIO61, IRQ BOTHEDGE) · VOL+ (TCA P10) · VOL- (TCA P11) · POWER (TCA P17, idle high). Không có núm xoay/scroll wheel.
- **TCA P13 = pulse output** (PWR_PULSE, 15×100ms) do code phát ra để lệnh tắt máy — không phải nút. Nút "giữa" trên ảnh marketing = P13 = không bấm được.
- **3 Virtual Keys ("Virtual Buttons" = 3 vạch cam cạnh phải)** là FT6336 báo điểm **ngoài vùng màn hình** (Y=1600, X=100/200/300) → vk_home/vk_prev/vk_next, không phải phần cứng.
- **VOL+/VOL- = page_prev/page_next khi đang đọc (đúng ảnh: cạnh phải "dễ cảm nhận" = lướt trang), mới = volume khi ở menu** — semantic kép từ code `lv_adapter_epdiy.cc`.
- BOOT long = shutdown; POWER short = màn hình tắt/đánh thức; VOL+ long = vol max (hoặc page_prev liên tục khi đọc); VOL- long = mute (hoặc page_next).
- **TCA9555 expander** (I2C 0x20, INT=GPIO2) phải được mô phỏng chính xác trong simulator (mỗi phím = bit port).

## 5. Blockers & Open Questions

- **gh token của `magicxlll` invalid** → user cần `! gh auth login -h github.com` (hoặc sửa token) trước khi auto-backup được. Chạy `gh auth status` để check.
- `RickyOS-Plus-1.2.9-full.bin` là **không phải** firmware chính thức phát hành (trang demo ghi "chính thức chưa phát hành") — là vật phẩm tham khảo.
- CloudZao fw có feature AI voice (Baiwen) cần Wi-Fi/MQTT/WebSocket — pha 2, stub no-op trong pha 1.
- ESP32-S31 là chip RISC-V khá mới — cần confirm có SDK/ESP-IDF v6.1 build được cho target này chưa, hay phải dùng ESP32-S3 (hiếm khi S31 = S3 rename). **Kiểm tra trong lúc clone.**

## 6. File Map (dự án, khi start)

- `README.md` — doc chính
- `docs/` — ADRs, intent, spec pha 1
- `.claude/skills/` — skill dự án (phong cách `agent-skills`)
- `.claude/rules/` — rules dự án
- `Project_memory.md` — file này (đặt ở repo root, không phải `memory/` cục bộ)
- `vendor/CloudZao/` (hoặc `simulator/`) — source fw + shim
- `fw-reference/` — RickyOS .bin + output REA
- `sim/` — code simulator (SDL2 + shim)
