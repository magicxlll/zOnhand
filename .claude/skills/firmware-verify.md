# Skill: firmware-verify (chạy test scenario trên sim)

> Chạy/đánh giá test scenarios (spec §4-T9) theo acceptance criteria §5.

## When to use
- Sau khi hoàn thành task T7 (book reader flow) hoặc T9 (test scenario).
- User yêu cầu "test lại tính năng X trên máy ảo."

## Steps
1. Xác định scenario trong `sim/tests/` (nếu đã có) hoặc `docs/rickycap-spec.md` (nếu đang từ spec RickyOS).
2. Chạy: `./sim/build.sh && ./build/sim --test <scenario>` (flag theo design T9; chưa có → chạy manual).
3. So sánh kết quả với acceptance criteria `docs/spec-phase-1.md` §5 (boot, keys, touch, reading, settings, NVS).
4. Ghi pass/fail + log vào `Project_memory.md`; nếu fail → liên kết với task chưa xong (T1–T8).
5. Nếu scenario có "machine-truth" (đối chiếu máy thật): **user chưa có máy thật** → chỉ so với docs + RickyOS bằng chứng (rule §1.4).

## Anti-patterns
- "Deemed" pass vì chưa chạy (phải có output thật).
- So với hành vi "máy thật mà ta đoán" (vi phạm rule §1.4).
