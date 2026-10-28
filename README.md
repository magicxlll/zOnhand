# zOnhand — trình máy ảo Metalio E-INK 4.7 Plus

Mô phỏng phần cứng **Metalio E-INK 4 Plus** (ESP32-S31, e-paper 4.7" 684×1216) trên macOS để **test firmware trước khi flash máy thật**, xây trên source firmware gốc [CloudZao/Metalio-E-INK4-Plus](https://github.com/CloudZao/Metalio-E-INK4-Plus.git) (ESP-IDF v6.1, C/C++, LVGL, MIT). Kiến trúc: **SDL2 native + shim ESP-IDF host-sim** — đúng model [crosspoint-simulator](https://github.com/crosspoint-reader/crosspoint-simulator).

## Đọc trước (thứ tự)

1. **`Project_memory.md`** — state ngắn, cập nhật sau mỗi debug (đọc trước hết).
2. **`docs/spec-phase-1.md`** — spec + plan + tasks (T1–T13) + acceptance + blockers. **Nếu là agent tiếp tục dự án: bắt đầu ở đây.**
3. **`.claude/rules/project-rules.md`** — rules bất di bất dịch (không sửa fw, không đoán GPIO, commit+push sau mỗi task).
4. `docs/adr/0001-sdl2-native-sim.md` — quyết định kiến trúc + bằng chứng.
5. `docs/ref/source-scan-2026-10-28.md` — evidence scan 4 nguồn (REA, reader-simulator, CloudZao, RickyOS).

## Layout repo

```
vendor/cloudzao/            # source fw (không sửa)
firmware-reference/         # RickyOS .bin + output REA
sim/                        # (T1+) code simulator: shim ESP-IDF host + SDL2
docs/                       # spec, adr, ref
.claude/skills/             # simulator-build, rickycap-research, firmware-verify
.claude/rules/             # project-rules.md
Project_memory.md           # memory cục bộ (cập nhật liên tục)
```

## Build / chạy

- Sim: `cd sim && ./build.sh` (T1 — chưa tồn tại).
- Flash thật (không phá): `cd vendor/cloudzao && idf.py build` (ESP-IDF v6.1, target `esp32s31`).

## Git

Backup: `https://github.com/magicxlll/zOnhand.git` (git HTTPS, token local trong `.git/.git-credentials-local` — không commit). Mỗi task = 1 commit + push.
