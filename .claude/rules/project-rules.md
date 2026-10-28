# Rules dự án zOnhand

> Mọi phiên (human hay agent) làm việc trong repo này phải tuân thủ. Đọc `Project_memory.md` trước, đọc `docs/spec-phase-1.md` khi bắt đầu làm.

## 1. Nguyên tắc bất di bất dịch

1. **Firmware source CloudZao (`vendor/cloudzao/`) KHÔNG được sửa** trừ khi task ghi rõ "phải sửa" + có approval của user. Mục tiêu là "chạy fw thật, shim phần cứng bên dưới."
2. **Không "đoán" GPIO/behavior** — mọi input mapping, behavior, partition offset phải trích từ source (pin SHA trong `docs/spec-phase-1.md` §9). Source code là ground-truth, không phải ảnh marketing (trừ kích thước vật lý 71×122×7.8mm).
3. **Cùng commit = cùng hành vi:** bất kỳ build sim nào phải giữ `idf.py build` (bản flash thật) pass. Đừng phá pipeline gốc vì sim.
4. **User chưa có máy thật** — baseline hành vi = docs + code CloudZao + bằng chứng RickyOS (đã chạy tốt máy thật). Đừng "trả lời" bằng việc đoán máy thật hành xử thế nào.
5. **Không commit token/secret.** Token gh nằm trong `.git/.git-credentials-local` (ngoài working tree, đã chmod 600). `gh` CLI không authenticate được (token thiếu scope `read:org`) — dùng `git` HTTPS trực tiếp.

## 2. Workflow bắt buộc

- **Trước khi code task nào:** đọc task đó trong `docs/spec-phase-1.md` §4 (Est, files chạm, acceptance).
- **Sau mỗi task/debug:**
  1. commit (msg conventional: `feat(sim): T3 EPD SDL2 renderer`, `fix(sim): ...`, `docs: ...`).
  2. `git push origin main`.
  3. cập nhật `Project_memory.md` (gì đã chạy, gì đã fix, gì còn dở, next action).
  4. đánh dấu `[x]` task trong `docs/spec-phase-1.md` §4.
- **Khi gặp blocker (B1–B7 trong spec):** ghi rõ trong `Project_memory.md` + escalate user, không tự "bỏ qua" bằng cách sửa fw hoặc đổi kiến trúc (trừ khi spec ghi rõ).

## 3. Phạm vi phase (đừng vượt)

- Phase 1 (hiện tại): boot → EPD → keys/touch → reading → settings + NVS/RTC/gauge. **Không** Wi-Fi/cloud, **không** AI voice, **không** BT audio, **không** frontlight — đó là Phase 2/3, T11+.
- Web/WASM/iOS/Windows: **out of scope hoàn toàn** cho Phase 1.

## 4. Style code

- C++ (fw) giữ style fw (4-space, `constexpr`, ESP_LOG*). Shim host: C++17, STL (`std::mutex`, `std::thread`, `std::condition_variable`), không thêm dependency ngoài SDL2 + CMake.
- Mỗi shim file (S1–S12 trong spec §3.2) là 1 module, không trộn lẫn API.
- Comment giải thích "tại sao" (đặc biệt mapping GPIO), không "cái gì."
