#!/usr/bin/env bash
# ==============================================================================
# zOnhand Simulator Launcher
# Default: dpReader31 (Metalio E-Ink 4 Plus, 684x1216 4bpp, Tiếng Việt)
# Legacy:  CloudZao ebook_sim (--legacy)
# ==============================================================================

set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/.." && pwd)"

if [ "${1:-}" = "--legacy" ] || [ "${1:-}" = "--cloudzao" ]; then
    shift
    EMU="$HERE/ebook-emulator"
    VENDOR="$ROOT/vendor/cloudzao"
    FONT="$HERE/ebook-emulator/assets/fonts/misans_25_2.ef"
    [ -r "$FONT" ] || { echo "font missing: $FONT" >&2; exit 1; }

    export EBOOK_LIBDIR="${EBOOK_LIBDIR:-$HERE/sample}"
    export EBOOK_FONTDIR="${EBOOK_FONTDIR:-$ROOT/vendor/cloudzao/tools/epdfont/MiSans-Light}"

    if [ ! -x "$EMU/build-ebook/bin/ebook_sim" ]; then
        echo ">> building legacy ebook_sim..."
        cmake -B "$EMU/build-ebook" -S "$EMU" \
            -DESP_MAIN="$VENDOR/main" -DCONFIG_LV_BUILD_DEMOS=OFF -DCONFIG_LV_BUILD_EXAMPLES=OFF >/dev/null
        make -C "$EMU/build-ebook" ebook_sim -j"$(sysctl -n hw.ncpu 2>/dev/null || nproc)"
    fi

    if [ ! -f "$HERE/sample/sample.ebook" ]; then
        mkdir -p "$HERE/sample"
        printf 'zOnhand sample book\n\nNative LVGL + CloudZao reader core on macOS.\n\nPage-turn test line two. Line three.\n' > "$HERE/sample/sample.txt"
        ( cd "$VENDOR/tools/ebook" && python3 convert_ebook.py "$HERE/sample/sample.txt" -o "$HERE/sample/sample.ebook" )
    fi

    BOOK="${1:-}"
    echo ">> running legacy ebook_sim..."
    exec env EBOOK_EPDFONT="$FONT" EBOOK_PATH="$BOOK" EBOOK_LIBDIR="$EBOOK_LIBDIR" EBOOK_FONTDIR="$EBOOK_FONTDIR" \
        "$EMU/build-ebook/bin/ebook_sim" -b sdl
fi

# Primary simulator: dpReader31
DP_SIM_DIR="$HERE/dpReader-sim"
if [ ! -x "$DP_SIM_DIR/dpReader_sim" ]; then
    echo ">> Building dpReader_sim..."
    make -C "$DP_SIM_DIR" -j"$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"
fi

echo ">> Running dpReader_sim (Metalio E-Ink 4 Plus)..."
cd "$DP_SIM_DIR"
exec ./dpReader_sim "$@"
