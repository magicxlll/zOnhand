#!/usr/bin/env bash
# ==============================================================================
# zOnhand - dpReader31 E-Ink Simulator Launcher (local)
# Double-click this file in macOS Finder to launch the simulator.
# ==============================================================================

set -e
DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$DIR"

if [ ! -f "dpReader_sim" ]; then
    echo ">> Đang biên dịch máy ảo dpReader_sim..."
    make -j"$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"
fi

./dpReader_sim "$@"
