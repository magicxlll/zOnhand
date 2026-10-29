# RickyOS-Plus-1.2.9-full.bin — REA Feature List (recommended)

> Reverse-engineering analysis (REA) of `firmware-reference/RickyOS-Plus-1.2.9-full.bin`.
> Methodology: string/region analysis only — **no disassembly, no machine-code dumps** (per project rule: source-grounded, no speculation).
> Cross-referenced against the CloudZao open-source scan (`docs/ref/source-scan-2026-10-28.md`) and the local `vendor/cloudzao` tree.

---

## One-line verdict

`RickyOS-Plus-1.2.9-full.bin` (16,769,024 bytes = exactly 16 MiB) is a **raw ESP32-S3 flash image** (RISC-V, `0xE9` OTA-magic first byte), **NOT** an ELF. It is the **CrossPoint Reader** firmware, rebranded/distributed as **RickyOS / "Ricky AI Studio"** by chinoryunqin, targeting the **Metalio E-Ink4 Plus** (`epd_eink4p`, 684×1216). It was **built with Arduino-ESP32 4.0.0-RC1 (PlatformIO), not the ESP-IDF v6.1 CMake stack** used by CloudZao — i.e. it is **a different codebase from `vendor/cloudzao`**, sharing only the hardware target and the e-book-reader domain.

---

## Image structure (evidence)

| Item | Evidence |
|---|---|
| Size | 16,769,024 B = 16 MiB exactly (matches ref-scan §Metalio "16MB flash") |
| First 16 B | `e9 03 02 4f ba a2 06 2f ee 00 00 00 20 00 00 00` — `0xE9` is ESP32 OTA image magic, `0x03` app subtype; consistent with RISC-V little-endian |
| ELF magic `\x7fELF` | **0 occurrences** in the whole image → no ELF, packed raw flash image |
| Entropy map | Data ~0x000000–0x058000 (entropy 5.6–8.0), then near-all-zero / 0xFF to end; tail at 0xFFDFDE = 0xFF padding → typical "app + partition padding" layout |
| `file(1)` | Reports "DOS executable (COM)" — **false positive** from the leading `0xE9`; ignore |
| CPU | `.local/arduino-esp32-4.0.0-RC1/cores/esp32/*.c` build paths embedded → ESP32-S3 (RISC-V, Xtensa-ported), **Arduino framework** |

---

## Feature table

Evidence = the exact string/symbol found by `strings -n 8` + `grep`. "Present?" reflects whether the feature is compiled into this image.

| Feature | Evidence (exact string + where) | Present? | Notes |
|---|---|---|---|
| Core e-book reader (EPUB/TXT, 3×3 cover grid) | ` CrossPoint Reader `, `book=%08x source=epub kind=generated cover=%s result=ready`, `%u books`, `/kindle.epub` | **yes** | Domain core, matches CloudZao |
| E-book library + index | `/.crosspoint/library`, `/.crosspoint/library.idx`, `%s, open a book, take your time.` | yes | |
| KOReader import | `KOReader`, `/.crosspoint/koreader.json`, `application/vnd.koreader.v1+json` | **yes** | |
| WeRead cloud book sync | `https://weread.qq.com`, `/.crosspoint/weread/shelf.bin`, `/.crosspoint/weread/browse`, `/web/shelf/sync`, `/web/book/bookmarklist?bookId=%s`, `/api/auth/getLoginUid` | **yes** | **absent in CloudZao open-source** |
| KOReader cloud sync | `https://sync.koreader.rocks:443`, `/syncs/progress/`, `Aborting sync because current progress could not be saved` | **yes** | **absent in CloudZao open-source** |
| OPDS / book catalogue | `/.crosspoint/opds.json`, `<D:resourcetype>`, `All %u page buffer slots full` (OPDS feed) | **yes** | |
| Cloud dictionary download | `https://rickyos-1309217881.cos.ap-guangzhou.myqcloud.com/dictionaries/m%u/cos.json`, `dictzip extraction failed`, `.dict.dz`, `DictionaryDownload` | **yes** | **absent in CloudZao open-source** |
| Cloud font / wallpaper download | `…/fonts/cos.json`, `…/wallpapers/cos.json`, `https://cdn.jsdelivr.net/gh/chinoryunqin/RickyOS-fonts@latest/mirror.json`, `Downloaded %u standby pictures and %u transparent wallpapers` | **yes** | |
| **OTA / firmware auto-update** | `https://chinoryunqin.github.io/RickyOS-site/plus/ota.json`, `…/cos…/ota/plus/ota-notes.txt`, `esp_ota_set_boot_partition failed`, `OTA app partition slot %d`, `Firmware Update in Settings checks for RickyOS releases` | **yes** | Distinct from CloudZao's `ota.cc` (CloudZao's endpoints are "empty" per ref-scan) |
| WiFi (STA + AP + hotspot) | `esp_wifi_start failed`, `ERROR: Failed to start Access Point!`, `Create Hotspot`, `Hotspot Mode`, `mdns_browser`, `_wifi_is_lr_enabled` | **yes** | |
| Built-in WebServer / browser file transfer | `CrossPointWebServer`, `/api/files`, `/api/status`, `/api/wifi`, `Browser transfer`, `FileBrowser`, `localhost` | **yes** | USB-MSC/AP transfer, same class as CloudZao AP |
| Frontlight (dual cool/warm) | `apply: brightness=%u level=%u totalDuty=%u coolDuty=%u warmDuty=%u lit=%d`, `FrontlightMgr`, `frontlightWarmth`, `ledc_set_duty`, `Frontlight resumed: on=%u` | **yes** | Dual-channel LEDC = same hardware class as CloudZao frontlight |
| Standby / sleep / power-off screens | `/.crosspoint/standby-source.txt`, `/poweroff.bmp`, `/sleep.bmp`, `@/.crosspoint/sleep_frame.bin`, `Auto standby`, `Choose power-off image`, `DEEPSLEEP` | **yes** | |
| Auto page turn / standby timer | `Auto page turn`, `Enter standby after this many idle minutes; Never disables the timer.` | yes | |
| **Reading gamification: "dog companion" (小狗伴读) + achievements** | `/.crosspoint/achievements.json`, `Achievements`, `Achievement popups`, `AchievementPopups`, `Enable achievements`, `rickyCompanion`, `Daily Goal`, `dailyGoalTarget`, `Goal met. Happy dog`, `Goal reached today. The dog approves`, `No reading yet today, so the dog is napping`, `Your longest companion: "%s"` | **yes** | **RickyOS-unique vs CloudZao open-source** |
| **Gomoku (五子棋) game with AI + "power skills"** | `/.crosspoint/gomoku.bin`, `/.crosspoint/gomoku_stats.bin`, `skill_gomoku_v1_a.bin`, `GomokuMenu`, `GomokuMode`, `Tap New for a fresh game`, `A full board or 400 actions is a draw.`, `Catch a breath: gain 3 energy, end turn.`, `Each power skill is usable once per player per game.` | **yes** | **RickyOS-unique** |
| Audio recording (wav) | `/Recordings/REC_%s_%02u.wav`, `Next recordings`, `Previous recordings`, `No recordings yet` | **yes** | WAV capture; note distinct from CloudZao AI-voice pipeline |
| Stickers / avatars | `ricky-avatar-a.bmp`, `ricky-avatar-b.bmp`, `avatar|%s|%u|%d,%d,%d`, `Change avatar`, `/.crosspoint/sticker-source.txt` | yes | |
| Reading stats / daily goal tracking | `/.crosspoint/reading_stats.json`, `Daily Reading (Last 7 days)`, `%u books finished this month`, `How consistently you showed up…` | yes | |
| Crash logging / diagnostics | `/crash_report.txt`, `A detailed report was saved to crash_report.txt`, `/rickyos-log.txt` | yes | |
| EPD panel driver (E-Ink4 Plus) | `epd_eink4p`, `E (…): EPD bus restore failed`, `TCA9555 not found; continue (panel power unavailable)`, `deferred partial start`, `B/W` refresh | **yes** | Matches CloudZao panel + TCA9555 expander |
| Touch (out-of-band virtual keys) | `Touch sensor not available on this chip`, `overlay refresh failed` | yes (generic) | No FT6336-specific string surfaced |

### Feature parity summary (RickyOS .bin vs CloudZao open-source)

- **Shared (both have):** e-book reader core, EPD panel + TCA9555, frontlight, WiFi + AP/WebServer transfer, OTA, standby/sleep.
- **In the .bin, NOT in CloudZao open-source:** WeRead cloud sync, KOReader cloud sync, OPDS catalogue, cloud dictionary/font/wallpaper downloads from `rickyos-*.cos.myqcloud.com` + `chinoryunqin` GitHub mirrors, the **dog-companion + achievements gamification**, and the **Gomoku AI game**.
- **Different build lineage:** .bin is **Arduino-ESP32 4.0.0-RC1 / PlatformIO** (CrossPoint), CloudZao is **ESP-IDF v6.1 CMake**. They are *not* the same source tree.

---

## Recommended inclusion for zOnhand Phase 1

Per spec §2/§1 (Phase 1 = boot → EPD → keys/touch → reading → settings + NVS/RTC/gauge; Wi-Fi/cloud, AI voice, BT audio, frontlight are Phase 2/3):

**In scope for the VM (verify these behaviors, but keep CloudZao source):**
- E-book reader flow (cover grid → detail → body), `.epub/.txt` (+ `.ebook` pipeline) — core Phase 1.
- Touch + physical-key page-turn, EPD full/partial refresh — core Phase 1.
- **WeRead/KOReader sync** and **cloud dictionaries/fonts** are **Phase 2** material — worth *noting* in the fork spec (`docs/rickycap-spec.md`) but not implemented in Phase 1.

**Out of scope / defer (note only):**
- **Dog-companion gamification + achievements + daily goal** — RickyOS-unique, most "RickyOS-flavored" features; strong Phase-2/3 candidate (spec B7: it is "spec sống", not to be ported blindly).
- **Gomoku AI game** — niche, defer.
- **Audio recording (wav)** — relates to Phase-3 BT audio.
- **OTA auto-update** — Phase 2+; Phase 1 must not add network update paths.

These give T9 (`test scenario`) concrete, source-cited scenarios to assert: bookmark lists, daily-goal messaging, standby image selection, frontlight dual-duty values.

---

## Unknowns / what the user should verify

1. **Is this actually a CrossPoint Reader fork or a clean-room port?** The strings are unmistakably CrossPoint (`/.crosspoint/*`, `CrossPointWebServer`, KOReader/WeRead). Confirm the legal/licensing position before treating its feature list as "the" RickyOS spec.
2. **Build toolchain version** — `.local/arduino-esp32-4.0.0-RC1` is a *precompiled* toolchain path; the RC1 suffix suggests an unreleased Arduino-ESP32. Verify whether it's a known-good or custom build.
3. **Partition table & offsets** are not extractable from strings alone; the user should confirm the actual partition layout (NVS, spiffs, `/.crosspoint` storage) matches the Metalio E-Ink4 Plus 16 MiB device before any VM image-layout work.
4. **"小狗伴读" (dog-companion) exact semantics** — strings show the *messages* and *data files* but not the game rules. The user should confirm intended behavior from the demo/install page (`rickyaistudio.com`) before spec'ing it.
5. **WeRead account model** — `/api/auth/getLoginInfo?uid=%s&otp=` implies OTP/uid-based auth; verify whether it's first-party Tencent WeRead or a proxy, and whether it's acceptable in a personal fork.
6. **Full binary integrity** — this analysis is string-based; it cannot confirm which features are *enabled by default* vs. compiled-in-but-gated. Runtime behavior still requires a real device or a VM of the actual source (which is not available).
