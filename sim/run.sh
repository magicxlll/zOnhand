#!/usr/bin/env bash
# zOnhand simulator launcher (Phase 1: reader core, native LVGL/SDL on macOS).
#
# Runs the CloudZao reader core (main/reader) against a native LVGL/SDL build.
# Panel is currently the harness default (480x800) — T3 will retarget to
# 684x1216 (Metalio E-Ink 4 Plus). This script only wires the inputs.
#
# Usage:
#   ./run.sh [book.ebook]     # book defaults to a generated sample
#
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/.." && pwd)"
EMU="$HERE/ebook-emulator"
VENDOR="$ROOT/vendor/cloudzao"

# Real .ef font lives here (upstream emulator/assets/fonts/*.ef are broken symlinks).
FONT="$HERE/ebook-emulator/assets/fonts/misans_25_2.ef"
[ -r "$FONT" ] || { echo "font missing: $FONT" >&2; exit 1; }

# Build on demand.
if [ ! -x "$EMU/build-ebook/bin/ebook_sim" ]; then
  echo ">> building ebook_sim (first run)..."
  cmake -B "$EMU/build-ebook" -S "$EMU" \
    -DESP_MAIN="$VENDOR/main" -DCONFIG_LV_BUILD_DEMOS=OFF -DCONFIG_LV_BUILD_EXAMPLES=OFF >/dev/null
  make -C "$EMU/build-ebook" ebook_sim -j"$(sysctl -n hw.ncpu 2>/dev/null || nproc)"
fi

# Sample book if none given.
BOOK="${1:-$HERE/sample/sample.ebook}"
if [ ! -f "$BOOK" ]; then
  mkdir -p "$HERE/sample"
  printf 'zOnhand sample book\n\nNative LVGL + CloudZao reader core on macOS.\n\nPage-turn test line two. Line three.\n' > "$HERE/sample/sample.txt"
  ( cd "$VENDOR/tools/ebook" && python3 convert_ebook.py "$HERE/sample/sample.txt" -o "$BOOK" )
fi

echo ">> running: $EMU/build-ebook/bin/ebook_sim  (SDL window will open)"
# -W/-H omitted: default is EPD_SIM_VIEW_W x EPD_VER_RES (728x1216 = panel + vstrip).
exec env EBOOK_EPDFONT="$FONT" EBOOK_PATH="$BOOK" \
  "$EMU/build-ebook/bin/ebook_sim" -b sdl
