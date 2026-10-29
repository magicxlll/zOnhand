#!/usr/bin/env bash
# ==============================================================================
# zOnhand - dpReader31 E-Ink Simulator Launcher for macOS
# Double-click this file in macOS Finder to launch the simulator.
# ==============================================================================

set -e

# Resolve directory where this script is located
DIR="$(cd "$(dirname "$0")" && pwd)"
SIM_DIR="$DIR/sim/dpReader-sim"

echo "=========================================================="
echo "   zOnhand - dpReader31 E-Ink Simulator (Metalio 4 Plus)  "
echo "=========================================================="
echo ""

# Check dependencies (SDL2)
if ! pkg-config --exists sdl2 2>/dev/null && [ ! -d "/opt/homebrew/include/SDL2" ] && [ ! -d "/usr/local/include/SDL2" ]; then
    echo "[!] Cảnh báo: Chưa tìm thấy thư viện SDL2 trên máy."
    echo "    Vui lòng cài đặt bằng lệnh: brew install sdl2 libpng"
    echo ""
fi

cd "$SIM_DIR"

# Build if binary doesn't exist
if [ ! -f "dpReader_sim" ]; then
    echo ">> Đang biên dịch máy ảo lần đầu (make)..."
    make -j"$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"
    echo ">> Biên dịch hoàn tất!"
    echo ""
fi

echo ">> Đang khởi chạy máy ảo dpReader31..."
echo "----------------------------------------------------------"
echo "  Điều khiển:"
echo "  - Chuột: Chạm màn hình E-Ink & bấm 4 nút viền bên phải"
echo "  - Phím K / Mũi tên Lên / PageUp:   Nút Cuộn Lên (VOL+)"
echo "  - Phím J / Mũi tên Xuống / PageDown: Nút Cuộn Xuống (VOL-)"
echo "  - Phím A / M:                       Nút Trợ lý / Menu (AI)"
echo "  - Phím P / ESC:                     Nút Nguồn / Khóa (POWER)"
echo "  - Phím Q:                           Thoát máy ảo"
echo "----------------------------------------------------------"
echo ""

./dpReader_sim "$@"

echo ""
echo ">> Máy ảo đã đóng."
