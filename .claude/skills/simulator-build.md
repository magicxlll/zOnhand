# Skill: simulator-build

> Build, chạy, debug trình máy ảo (sim) CloudZao theo `docs/spec-phase-1.md` §4 (task T1–T8).

## When to use
- Cần build hoặc chạy lại sim (`sim/build.sh`).
- Gặp crash/build error trong task T1–T8.
- Cần wire thêm 1 shim module (S1–S12).

## Steps
1. `cd /Volumes/ZMac1TB/Workspace/StudioProjects/zOnhand/sim && ./build.sh` — kiểm tra trước.
2. Nếu build fail: đọc error, đối chiếu `docs/spec-phase-1.md` §3.2 (module shim cần) + §6 (blockers B1–B7).
3. Nếu run crash: `ESP_LOG_LEVEL=DEBUG` (nếu có env) để bật log; kiểm tra `metalio_keys.cc` init log có khớp `metalio_keys.cc` thật không (T4 acceptance).
4. Sau mỗi fix: commit + push + cập nhật `Project_memory.md` + đánh dấu task (rule §2).
5. Gặp B1 (ESP-IDF host-sim không build được) → escalate user, đề xuất fork sang PlatformIO **trước khi code**.

## Anti-patterns
- Sửa source `vendor/cloudzao/` để "dễ build" (vi phạm rule §1).
- Bỏ qua `patches/esp-idf/` khi build flash thật (B3).
- Dùng `gh` CLI cho git ops (token thiếu scope, fail).
