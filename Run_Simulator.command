#!/bin/bash
# ==============================================================================
# zOnhand Simulator Launcher for macOS Finder (Double-Click to Run)
# ==============================================================================
DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$DIR/sim"

echo "Đang khởi động máy ảo zOnhand (Metalio E-Ink 4 Plus)..."
./run.sh
