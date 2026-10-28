# Skill: rickycap-research (REA phân tích RickyOS)

> Phân tích `firmware-reference/RickyOS-Plus-1.2.9-full.bin` bằng REA → `docs/rickycap-spec.md` (danh sách tính năng + evidence + đề xuất impl).

## When to use
- Bắt đầu REA workflow (spec §7).
- Cần evidence "tính năng X có thật trong RickyOS, đã chạy máy thật."

## Steps
1. Kiểm tra Node: `node --version` (cần 22.x/24.x/26.x — hiện v26.12.1 OK).
2. Setup: `npx rea-agents@latest setup` (chọn "Claude Code" nếu prompt; restart session nếu cần).
3. REA analyze `.bin`: regions, strings, symbols, native handoffs. Lệnh mẫu (qua MCP tools của REA sau setup): "extract top-level regions + strings suggesting feature names (checklist, wallpaper, companion, transfer, ota)".
4. Đối chiếu với `https://rickyaistudio.com/install.html` (feature công khai đã scan 2026-10-28, xem `docs/ref/`).
5. Viết `docs/rickycap-spec.md`: bảng (Tính năng | Evidence trong .bin/demo | Phase đề xuất (2/3) | Ghi chú impl). **Không** ghi "sao chép code" — chỉ spec + test scenario.
6. **User review từng mục trước khi đưa vào plan Phase 2/3** (rule: user là gate).
7. Commit `docs/rickycap-spec.md` + cập nhật `Project_memory.md`.

## Anti-patterns
- Dùng .bin làm "nguồn code" (không có source, chỉ trích spec).
- Bỏ qua evidence (mỗi tính năng phải dẫn được strings/regions cụ thể).
