# dpReader31 E-Ink Simulator (Metalio E-Ink 4 Plus)

Trình giả lập macOS native cho firmware **dpReader31** (`v1.1.6-vn`, Tiếng Việt), mô phỏng đầy đủ phần cứng máy đọc sách **Metalio E-Ink 4 Plus** (ESP32-S31 + Màn hình EPD 684×1216 4-bit Grayscale).

---

## 1. Cách chạy nhanh trên macOS

### Cách 1: Click đúp chuột từ Finder (Dành cho người dùng Mac)
- Click đúp vào file [`Chay_dpReader.command`](file:///Volumes/ZMac1TB/Workspace/StudioProjects/zOnhand/Chay_dpReader.command) ở thư mục gốc dự án.
- Máy sẽ tự động mở Terminal, kiểm tra thư viện SDL2, biên dịch (nếu cần) và khởi chạy cửa sổ máy ảo.

### Cách 2: Chạy từ dòng lệnh Terminal
```bash
# Ở thư mục gốc dự án:
./sim/run.sh

# Hoặc:
cd sim/dpReader-sim
./dpReader_sim
```

### Chạy kiểm tra tự động (Regression Smoke Test)
```bash
./sim/run.sh --test
```
Lệnh test sẽ tự động boot vào Trang chủ, chuyển sang tab Kệ sách, mở sách `sample.txt`, lật trang bằng nút phần cứng VOL- và xác nhận UI hoạt động hoàn hảo.

---

## 2. Hướng dẫn điều khiển máy ảo

| Thao tác | Mô tả |
| :--- | :--- |
| **Chuột trái vào màn hình E-Ink** | Cảm ứng chạm (Touch / Gesture CST836U) |
| **Click vào 4 nút viền bên phải** | Bấm 4 nút phần cứng Metalio: POWER, VOL+, VOL-, AI |
| **Phím `K` / `Mũi tên Lên` / `PageUp`** | Phím cứng cuộn lên / lật trang trước (**VOL+**) |
| **Phím `J` / `Mũi tên Xuống` / `PageDown`** | Phím cứng cuộn xuống / lật trang sau (**VOL-**) |
| **Phím `A` / `M`** | Phím tắt AI / Quick Menu (**AI**) |
| **Phím `P` / `ESC`** | Phím Nguồn / Khóa / Mở màn hình (**POWER**) |
| **Phím `Q`** | Thoát máy ảo |

---

## 3. Kiến trúc Giả lập (Simulator Architecture)

1. **Giao diện & Đồ họa (`sim_display.c` & `sim_main.c`):**
   - Giả lập framebuffer 4-bit Grayscale (`684 x 1216`), xoay 90° portrait chuẩn màn hình E-Ink Metalio.
   - Hiển thị trên cửa sổ SDL2 độ phân giải thực tế `800 x 1216` (684px màn hình E-Ink + 116px khung viền chứa 4 nút cứng ảo).
   - Thread-safe: bảo vệ main thread của macOS Cocoa khỏi xung đột AppKit event loop khi các tác vụ FreeRTOS chạy background.

2. **Cảm ứng & Nút cứng (`sim_touch.c` & `sim_hw.c`):**
   - Giả lập I2C chip cảm ứng `CST836U`.
   - Giả lập PMU Pico Microcontroller (`read_pico_pmu`) truyền trạng thái nút nguồn, pin, đèn nền.

3. **Hệ thống Lưu trữ & VFS (`sim_sd.c` & `sim_vfs_hook.h`):**
   - Ánh xạ đường dẫn thiết bị `/sdcard/...` và `/flash/...` sang thư mục máy tính cục bộ `./storage/...`.
   - Sách đặt trong `sim/dpReader-sim/storage/sdcard/books/` sẽ tự động hiển thị trên Kệ sách.

4. **NVS Non-Volatile Storage (`nvs_shim.cpp`):**
   - Lưu trữ cấu hình hệ thống, độ sáng, cỡ chữ, tiến độ đọc sách dạng JSON key-value có hỗ trợ duyệt iterator (`nvs_entry_find`).

5. **Phông chữ Tiếng Việt & TTF (`sim_assets.c`):**
   - Nhúng trực tiếp binary symbol của `builtin.ttf` (hỗ trợ Tiếng Việt và các ngôn ngữ) thông qua clang assembly directives tương thích Apple arm64.
