#include "book_reader_emulator.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <dirent.h>
#include <memory>
#include <string>
#include <sys/stat.h>
#include <vector>

#include "font_emulator.h"
#include "epdfont.h"
#include "book_session.h"
#include "ebook_document.h"
#include "reader_page_render.h"
#include "reader_types.h"
#include "nvs.h"
#include "nvs_flash.h"

// lv_sdl_mouse_create / lv_sdl_keyboard_create come in via lvgl.h (which pulls
// in src/drivers/lv_drivers.h). No extra include needed.

namespace {

constexpr lv_coord_t kListPad = 12;
constexpr lv_coord_t kFooterH = 36;
constexpr lv_coord_t kStatusPadV = 8;

struct ReaderUi {
    std::unique_ptr<reader::BookSession> session;
    epdfont_t* epdfont = nullptr;
    std::string epdfont_path;
    std::string last_error;
    std::string temp_ebook_path;
    std::string open_ebook_path;

    lv_obj_t* scr = nullptr;
    lv_obj_t* content = nullptr;
    lv_obj_t* footer = nullptr;
    lv_coord_t status_h = 0;
};

static bool s_ui_ready = false;

ReaderUi& Ui() {
    static ReaderUi s;
    return s;
}

void DisableScroll(lv_obj_t* obj) {
    if (obj == nullptr) {
        return;
    }
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
}

// ---------------------------------------------------------------------------
// T2' host app shell: Home (book library) + Settings (font size), layered on
// the reader core. "Rail" architecture — these are host UI additions; they do
// NOT touch reader::BookSession (the real CloudZao core). The device's true
// boot (Application::Start) is T2-full.
// ---------------------------------------------------------------------------
enum class ShellScreen { kHome, kSettings, kReader, kStandby };

// Available .ef font sizes (matches vendor tools/epdfont/MiSans-Light/).
constexpr int kFontSizes[] = {20, 22, 24, 25, 28, 30, 34, 40};
constexpr int kFontSizeCount = sizeof(kFontSizes) / sizeof(kFontSizes[0]);

struct ShellState {
    ShellScreen screen = ShellScreen::kHome;
    ShellScreen prev_screen = ShellScreen::kHome;
    int home_sel = 0;
    int settings_sel = 3;             // which setting row is focused (3 -> 25)
    int font_size_idx = 3;            // -> kFontSizes[3] = 25 (default, matches shipped .ef)
    int refresh_mode = 0;             // 0 = partial (default), 1 = full refresh
    std::string last_book;            // path of last read book from NVS
    std::vector<std::string> books;   // .ebook/.epub paths
    std::string font_dir;             // dir holding misans_*.ef
};

static ShellState& Shell() {
    static ShellState s;
    return s;
}

static void NvsLoad() {
    nvs_flash_init();
    nvs_handle_t h = 0;
    if (nvs_open("reader", NVS_READONLY, &h) == ESP_OK) {
        int32_t val = 0;
        if (nvs_get_i32(h, "font_idx", &val) == ESP_OK && val >= 0 && val < kFontSizeCount) {
            Shell().font_size_idx = val;
            Shell().settings_sel = val;
        }
        if (nvs_get_i32(h, "refr_mode", &val) == ESP_OK) {
            Shell().refresh_mode = val;
        }
        size_t len = 0;
        if (nvs_get_str(h, "last_book", nullptr, &len) == ESP_OK && len > 0) {
            std::vector<char> buf(len);
            if (nvs_get_str(h, "last_book", buf.data(), &len) == ESP_OK) {
                Shell().last_book = buf.data();
            }
        }
        nvs_close(h);
    }
}

static void NvsSaveFontIdx(int idx) {
    nvs_handle_t h = 0;
    if (nvs_open("reader", NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_i32(h, "font_idx", idx);
        nvs_commit(h);
        nvs_close(h);
    }
}

static void NvsSaveRefreshMode(int mode) {
    nvs_handle_t h = 0;
    if (nvs_open("reader", NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_i32(h, "refr_mode", mode);
        nvs_commit(h);
        nvs_close(h);
    }
}

static void NvsSaveLastBook(const std::string& book) {
    nvs_handle_t h = 0;
    if (nvs_open("reader", NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_str(h, "last_book", book.c_str());
        nvs_commit(h);
        nvs_close(h);
    }
}

static std::string GetBatteryString() {
    const char* env = std::getenv("ZONHAND_BATTERY_LEVEL");
    int lvl = 70;
    if (env != nullptr) {
        int v = std::atoi(env);
        if (v >= 0 && v <= 100) lvl = v;
    }
    return std::to_string(lvl) + "% 🔋";
}

static std::string GetTimeString() {
    time_t now = ::time(nullptr);
    struct tm local = {};
    if (localtime_r(&now, &local) != nullptr) {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%02d:%02d", local.tm_hour, local.tm_min);
        return buf;
    }
    return "12:00";
}

static std::string GetDateString() {
    time_t now = ::time(nullptr);
    struct tm local = {};
    if (localtime_r(&now, &local) != nullptr) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d", local.tm_year + 1900, local.tm_mon + 1, local.tm_mday);
        return buf;
    }
    return "2026-10-29";
}

static std::string ResolveFontDir() {
    const char* env = getenv("EBOOK_FONTDIR");
    if (env != nullptr && env[0] != '\0') {
        struct stat st;
        if (stat(env, &st) == 0 && S_ISDIR(st.st_mode)) {
            return env;
        }
    }
    const char* candidates[] = {
        "vendor/cloudzao/tools/epdfont/MiSans-Light",
        "../vendor/cloudzao/tools/epdfont/MiSans-Light",
        "../../vendor/cloudzao/tools/epdfont/MiSans-Light",
        "../../../vendor/cloudzao/tools/epdfont/MiSans-Light",
        "/Volumes/ZMac1TB/Workspace/StudioProjects/zOnhand/vendor/cloudzao/tools/epdfont/MiSans-Light",
    };
    for (const char* c : candidates) {
        struct stat st;
        if (stat(c, &st) == 0 && S_ISDIR(st.st_mode)) {
            return c;
        }
    }
    return "";
}

static std::string FontPathFor(int idx) {
    auto& sh = Shell();
    if (sh.font_dir.empty()) {
        sh.font_dir = ResolveFontDir();
    }
    if (sh.font_dir.empty() || idx < 0 || idx >= kFontSizeCount) {
        return "";
    }
    return sh.font_dir + "/misans_" + std::to_string(kFontSizes[idx]) + "_2.ef";
}

void BuildHomeScreen();
void BuildSettingsScreen();
bool OpenSessionPath(const char* path);

void EnsureEpdfont() {
    auto& ui = Ui();
    if (ui.epdfont != nullptr) {
        return;
    }
    if (!ui.epdfont_path.empty()) {
        ui.epdfont = epdfont_open(ui.epdfont_path.c_str());
    }
    if (ui.epdfont == nullptr) {
        auto& sh = Shell();
        std::string fallback = FontPathFor(sh.font_size_idx);
        if (!fallback.empty() && fallback != ui.epdfont_path) {
            ui.epdfont_path = fallback;
            epdfont_emulator_set_path(fallback.c_str());
            ui.epdfont = epdfont_open(fallback.c_str());
        }
    }
}

void ReleaseEpdfont() {
    auto& ui = Ui();
    if (ui.epdfont == nullptr) {
        return;
    }
    epdfont_close(ui.epdfont);
    ui.epdfont = nullptr;
}

const lv_font_t* UiFont() {
    EnsureEpdfont();
    if (Ui().epdfont != nullptr) {
        const lv_font_t* f = epdfont_get_lv_font(Ui().epdfont);
        if (f != nullptr) {
            return f;
        }
    }
    return LV_FONT_DEFAULT;
}

const lv_font_t* ListFont() {
    return UiFont();
}

const lv_font_t* BookFont() {
    return UiFont();
}

void PrewarmPageFont(const reader::Page* page) {
    auto& ui = Ui();
    if (ui.epdfont == nullptr || page == nullptr) {
        return;
    }
    std::string blob;
    blob.reserve(2048);
    for (const auto& item : page->items) {
        if (item.kind == reader::ContentKind::kText) {
            blob.append(item.text);
            blob.push_back('\n');
        }
    }
    if (!blob.empty()) {
        epdfont_prewarm_utf8(ui.epdfont, blob.c_str(), blob.size());
    }
}

lv_coord_t ContentWidth() { return EPD_HOR_RES - kListPad * 2; }

lv_coord_t StatusBarHeight() {
    const lv_font_t* ui_font = UiFont();
    const lv_coord_t line_h = ui_font != nullptr ? ui_font->line_height : 30;
    return line_h + kStatusPadV * 2;
}

lv_coord_t MeasureTextWidth(const lv_font_t* font, const char* text) {
    if (font == nullptr || text == nullptr) {
        return 0;
    }
    lv_coord_t w = 0;
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(text); *p; ) {
        uint32_t cp = 0;
        if ((*p & 0x80) == 0) {
            cp = *p++;
        } else if ((*p & 0xE0) == 0xC0 && p[1]) {
            cp = ((p[0] & 0x1F) << 6) | (p[1] & 0x3F);
            p += 2;
        } else if ((*p & 0xF0) == 0xE0 && p[1] && p[2]) {
            cp = ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
            p += 3;
        } else {
            cp = *p++;
        }
        w += lv_font_get_glyph_width(font, cp, 0);
    }
    return w;
}

std::string TruncateTextToWidth(std::string text, const lv_font_t* font, lv_coord_t max_w) {
    if (max_w <= 0 || MeasureTextWidth(font, text.c_str()) <= max_w) {
        return text;
    }
    while (!text.empty()) {
        text.pop_back();
        if (MeasureTextWidth(font, (text + "…").c_str()) <= max_w) {
            return text + "…";
        }
    }
    return "…";
}

void UpdateFooter() {
    auto& ui = Ui();
    if (ui.footer == nullptr || !ui.session || !ui.session->IsOpen()) {
        return;
    }
    char buf[96];
    const lv_font_t* footer_font = ListFont();
    const lv_coord_t footer_w = EPD_HOR_RES - 16;
    const int pct_x10 = ui.session->ReadingProgressX10();
    char suffix[24];
    std::snprintf(suffix, sizeof(suffix), " · %d.%d%%", pct_x10 / 10, pct_x10 % 10);
    const lv_coord_t suffix_w = MeasureTextWidth(footer_font, suffix);
    const lv_coord_t title_budget = std::max<lv_coord_t>(0, footer_w - suffix_w);

    std::string title = ui.session->CurrentTocTitle();
    if (title.empty()) {
        title = "正文";
    }
    title = TruncateTextToWidth(title, footer_font, title_budget);
    std::snprintf(buf, sizeof(buf), "%s%s", title.c_str(), suffix);
    lv_label_set_text(ui.footer, buf);
}

void RenderPage() {
    auto& ui = Ui();
    if (ui.content == nullptr || !ui.session) {
        return;
    }
    lv_obj_clean(ui.content);
    DisableScroll(ui.content);

    const reader::Page* page = ui.session->CurrentPageData();
    if (page == nullptr) {
        lv_obj_t* msg = lv_label_create(ui.content);
        lv_label_set_text(msg, "无法加载页面");
        return;
    }

    PrewarmPageFont(page);

    const bool page_images = ui.session->IsPageImagesMode();
    const lv_coord_t max_w = page_images ? EPD_HOR_RES : ContentWidth();
    const lv_coord_t content_h = lv_obj_get_height(ui.content);
    lv_obj_t* col = lv_obj_create(ui.content);
    lv_obj_remove_style_all(col);
    lv_obj_set_size(col, max_w, page_images ? content_h : LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(col, 0, 0);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(col, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(col, page_images ? 0 : reader::kReaderLineGapDefault, 0);
    lv_obj_align(col, page_images ? LV_ALIGN_TOP_LEFT : LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_clear_flag(col, LV_OBJ_FLAG_CLICKABLE);
    DisableScroll(col);

    for (const auto& item : page->items) {
        if (item.kind == reader::ContentKind::kText) {
            const lv_font_t* book_font = BookFont();
            if (item.text.empty()) {
                continue;
            }
            reader::CreatePageTextLabel(col, item, max_w, book_font, reader::kReaderLineGapDefault,
                                       reader::kReaderParaGapDefault);
        } else {
            reader::RasterImage img;
            const lv_coord_t max_h = page_images ? content_h : (content_h * 3 / 4);
            if (ui.session->LoadPageImage(item.image_href, max_w, max_h > 40 ? max_h : 200, img) &&
                !img.empty()) {
                auto* heap_img = new reader::RasterImage(std::move(img));
                heap_img->BindDsc();
                lv_obj_t* image = lv_image_create(col);
                lv_image_set_src(image, &heap_img->dsc);
                lv_obj_set_user_data(image, heap_img);
                lv_obj_add_event_cb(
                    image,
                    [](lv_event_t* ev) {
                        auto* p = static_cast<reader::RasterImage*>(lv_obj_get_user_data(
                            static_cast<lv_obj_t*>(lv_event_get_target(ev))));
                        delete p;
                    },
                    LV_EVENT_DELETE, nullptr);
                lv_obj_clear_flag(image, LV_OBJ_FLAG_CLICKABLE);
                if (page_images) {
                    lv_obj_align(image, LV_ALIGN_TOP_LEFT, 0, 0);
                }
            } else {
                lv_obj_t* label = lv_label_create(col);
                lv_label_set_text(label, "[图片]");
                lv_obj_set_style_text_font(label, BookFont(), 0);
            }
        }
    }
    UpdateFooter();
}

void OnContentClicked(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    auto& ui = Ui();
    if (!ui.session || !ui.session->IsOpen()) {
        return;
    }
    if (ui.session->NextPage()) {
        RenderPage();
    }
}

// ---------------------------------------------------------------------------
// T5: virtual-key strip (device right bezel: 3 out-of-band marks HOME/PREV/NEXT,
// FT6336 Y=1600 X=100/200/300). Rendered as an orange mark column OUTSIDE the
// 684x1216 e-paper region; the reader keeps its true panel size.
// ---------------------------------------------------------------------------
static constexpr int VK_HOME = 0, VK_PREV = 1, VK_NEXT = 2;

void OnVkClick(lv_event_t* e) {
    const int which =
        static_cast<int>(reinterpret_cast<std::intptr_t>(lv_event_get_user_data(e)));
    auto& sh = Shell();

    if (sh.screen == ShellScreen::kStandby) {
        book_reader_exit_standby();
        return;
    }

    if (sh.screen == ShellScreen::kHome) {
        const int max_sel = static_cast<int>(sh.books.size()) + 1;
        if (which == VK_PREV) {
            if (sh.home_sel > 0) {
                sh.home_sel--;
                BuildHomeScreen();
            }
        } else if (which == VK_NEXT) {
            if (sh.home_sel < max_sel) {
                sh.home_sel++;
                BuildHomeScreen();
            }
        } else if (which == VK_HOME) {
            if (sh.home_sel < static_cast<int>(sh.books.size())) {
                OpenSessionPath(sh.books[sh.home_sel].c_str());
            } else if (sh.home_sel == static_cast<int>(sh.books.size())) {
                book_reader_show_settings();
            } else {
                book_reader_enter_standby();
            }
        }
        return;
    }

    if (sh.screen == ShellScreen::kSettings) {
        const int max_settings_sel = kFontSizeCount + 1;
        if (which == VK_PREV) {
            if (sh.settings_sel > 0) {
                sh.settings_sel--;
                BuildSettingsScreen();
            }
        } else if (which == VK_NEXT) {
            if (sh.settings_sel < max_settings_sel) {
                sh.settings_sel++;
                BuildSettingsScreen();
            }
        } else if (which == VK_HOME) {
            book_reader_show_home();
        }
        return;
    }

    if (which == VK_HOME) {
        book_reader_vk_home();
    } else if (which == VK_PREV) {
        book_reader_prev_page();
    } else if (which == VK_NEXT) {
        book_reader_next_page();
    }
}

void BuildVStrip(lv_obj_t* root) {
    // Dark bezel column, right of the e-paper panel.
    lv_obj_t* bezel = lv_obj_create(root);
    lv_obj_remove_style_all(bezel);
    lv_obj_set_size(bezel, VSTRIP_W, EPD_VER_RES);
    lv_obj_align(bezel, LV_ALIGN_TOP_LEFT, EPD_HOR_RES, 0);
    lv_obj_set_style_bg_color(bezel, lv_color_hex(0x2b2b2b), 0);
    lv_obj_set_style_bg_opa(bezel, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(bezel, 0, 0);
    lv_obj_clear_flag(bezel, LV_OBJ_FLAG_CLICKABLE);
    DisableScroll(bezel);

    struct VkDef {
        int which;
        const char* label;  // H/J/K shortcut (see T4 key map)
        lv_coord_t y_frac4; // vertical slot (quarters of panel height)
    };
    const VkDef defs[3] = {
        {VK_HOME, "HOME\nH", 1},
        {VK_PREV, "PREV\nK", 2},
        {VK_NEXT, "NEXT\nJ", 3},
    };
    for (const auto& d : defs) {
        lv_obj_t* mark = lv_obj_create(root);
        lv_obj_remove_style_all(mark);
        lv_obj_set_size(mark, VSTRIP_W - 12, 88);
        lv_obj_align(mark, LV_ALIGN_TOP_LEFT, EPD_HOR_RES + 6, EPD_VER_RES * d.y_frac4 / 4 - 44);
        lv_obj_set_style_radius(mark, 8, 0);
        lv_obj_set_style_bg_color(mark, lv_color_hex(0xF0A500), 0);  // device orange
        lv_obj_set_style_bg_opa(mark, LV_OPA_COVER, 0);
        lv_obj_add_flag(mark, LV_OBJ_FLAG_CLICKABLE);
        DisableScroll(mark);
        lv_obj_t* t = lv_label_create(mark);
        lv_label_set_text(t, d.label);
        lv_obj_center(t);
        lv_obj_set_style_text_color(t, lv_color_hex(0x111111), 0);
        lv_obj_set_style_text_align(t, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_add_event_cb(mark, OnVkClick, LV_EVENT_CLICKED,
                           reinterpret_cast<void*>(static_cast<std::intptr_t>(d.which)));
    }
}

static void BuildTopStatusBar(lv_obj_t* parent, const char* title_text) {
    auto& ui = Ui();
    ui.status_h = StatusBarHeight();
    lv_obj_t* bar = lv_obj_create(parent);
    lv_obj_remove_style_all(bar);
    lv_obj_set_size(bar, EPD_HOR_RES, ui.status_h);
    lv_obj_align(bar, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_TRANSP, 0);
    DisableScroll(bar);

    // Left time label
    lv_obj_t* time_lbl = lv_label_create(bar);
    lv_label_set_text(time_lbl, GetTimeString().c_str());
    lv_obj_align(time_lbl, LV_ALIGN_LEFT_MID, 12, 0);
    lv_obj_set_style_text_font(time_lbl, ListFont(), 0);
    lv_obj_set_style_text_color(time_lbl, lv_color_hex(0x444444), 0);

    // Center title label
    lv_obj_t* title_lbl = lv_label_create(bar);
    lv_label_set_text(title_lbl, title_text ? title_text : "");
    lv_obj_align(title_lbl, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(title_lbl, ListFont(), 0);
    lv_obj_set_style_text_color(title_lbl, lv_color_black(), 0);

    // Right battery label
    lv_obj_t* bat_lbl = lv_label_create(bar);
    lv_label_set_text(bat_lbl, GetBatteryString().c_str());
    lv_obj_align(bat_lbl, LV_ALIGN_RIGHT_MID, -12, 0);
    lv_obj_set_style_text_font(bat_lbl, ListFont(), 0);
    lv_obj_set_style_text_color(bat_lbl, lv_color_hex(0x444444), 0);

    // Bottom hairline divider
    lv_obj_t* line = lv_obj_create(bar);
    lv_obj_remove_style_all(line);
    lv_obj_set_size(line, EPD_HOR_RES - 24, 1);
    lv_obj_align(line, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(line, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
}

void BuildScreen() {
    auto& ui = Ui();
    lv_obj_t* old_scr = ui.scr;
    ui.scr = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(ui.scr, lv_color_hex(0xEFEBE0), 0);
    lv_obj_set_style_bg_opa(ui.scr, LV_OPA_COVER, 0);
    lv_obj_set_style_text_font(ui.scr, ListFont(), 0);
    lv_obj_set_style_text_color(ui.scr, lv_color_black(), 0);
    DisableScroll(ui.scr);

    BuildTopStatusBar(ui.scr, "Metalio Plus · 阅读");

    const lv_coord_t body_h = EPD_VER_RES - ui.status_h - kFooterH;
    const bool page_images = ui.session && ui.session->IsPageImagesMode();
    lv_obj_t* body = lv_obj_create(ui.scr);
    lv_obj_remove_style_all(body);
    lv_obj_set_size(body, EPD_HOR_RES, body_h);
    // Left-anchor to the e-paper panel (x=0..EPD_HOR_RES); the vstrip occupies
    // x=EPD_HOR_RES.. so the reader stays panel-centered, not display-centered.
    lv_obj_align(body, LV_ALIGN_TOP_LEFT, 0, ui.status_h);
    lv_obj_set_style_pad_hor(body, page_images ? 0 : kListPad, 0);
    lv_obj_set_style_bg_opa(body, LV_OPA_TRANSP, 0);
    DisableScroll(body);
    lv_obj_add_event_cb(body, OnContentClicked, LV_EVENT_CLICKED, nullptr);
    ui.content = body;

    ui.footer = lv_label_create(ui.scr);
    lv_obj_set_width(ui.footer, EPD_HOR_RES - 16);
    lv_obj_set_style_text_align(ui.footer, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(ui.footer, ListFont(), 0);
    lv_obj_set_style_text_color(ui.footer, lv_color_black(), 0);
    lv_label_set_long_mode(ui.footer, LV_LABEL_LONG_CLIP);
    lv_obj_align(ui.footer, LV_ALIGN_BOTTOM_LEFT, 8, -4);
    DisableScroll(ui.footer);

    BuildVStrip(ui.scr);
    lv_screen_load(ui.scr);
    if (old_scr != nullptr && lv_obj_is_valid(old_scr)) {
        lv_obj_delete_async(old_scr);
    }
}

void ShowIdleScreen() {
    auto& ui = Ui();
    if (ui.session && ui.session->IsOpen()) {
        return;
    }
    if (ui.scr != nullptr && lv_obj_is_valid(ui.scr)) {
        lv_obj_delete_async(ui.scr);
    }
    ui.content = nullptr;
    ui.footer = nullptr;
    ui.scr = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(ui.scr, lv_color_hex(0xEFEBE0), 0);
    lv_obj_set_style_bg_opa(ui.scr, LV_OPA_COVER, 0);
    DisableScroll(ui.scr);

    const lv_font_t* font = UiFont();
    lv_obj_t* lbl = lv_label_create(ui.scr);
    lv_label_set_text(lbl, "墨水屏模拟器\n684 × 1216\n\n从书库打开 .ebook 预览");
    if (font != nullptr) {
        lv_obj_set_style_text_font(lbl, font, 0);
    }
    lv_obj_set_style_text_color(lbl, lv_color_hex(0x111111), 0);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    // Center on the e-paper panel only (not the full display incl. vstrip).
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, -VSTRIP_W / 2, EPD_VER_RES / 3);
    BuildVStrip(ui.scr);
    lv_screen_load(ui.scr);
}

bool OpenSessionPath(const char* path) {
    auto& ui = Ui();
    ui.last_error.clear();
    if (path == nullptr || path[0] == '\0') {
        ui.last_error = "空路径";
        return false;
    }
    ui.open_ebook_path = path;

    EnsureEpdfont();
    ui.session = std::make_unique<reader::BookSession>();
    ui.session->SetFont(BookFont());
    ui.session->SetGaps(reader::kReaderLineGapDefault, reader::kReaderParaGapDefault);

    const lv_coord_t body_h = EPD_VER_RES - StatusBarHeight() - kFooterH;
    const bool page_images = reader::EbookDocument::PeekPageImages(path);
    if (page_images) {
        ui.session->SetViewport(EPD_HOR_RES, body_h);
    } else {
        ui.session->SetViewport(ContentWidth(), body_h - 8);
    }

    reader::BookInfo info;
    info.path = path;
    info.format = reader::BookFormat::kEbook;
    info.title = path;

    if (!ui.session->Open(info)) {
        ui.last_error = "打开 .ebook 失败";
        ui.session.reset();
        return false;
    }

    Shell().screen = ShellScreen::kReader;
    Shell().last_book = path != nullptr ? path : "";
    NvsSaveLastBook(Shell().last_book);
    BuildScreen();
    RenderPage();
    return true;
}

// Scan a directory for readable e-book files (.ebook / .epub).
static std::vector<std::string> ScanBooks(const char* dir) {
    std::vector<std::string> out;
    std::vector<std::string> dirs;
    if (dir != nullptr && dir[0] != '\0') {
        dirs.push_back(dir);
        dirs.push_back(std::string(dir) + "/Books");
    }
    dirs.push_back("sample");
    dirs.push_back("sim/sample");
    dirs.push_back("../sample");
    dirs.push_back("../../sim/sample");

    for (const auto& d : dirs) {
        DIR* dp = ::opendir(d.c_str());
        if (dp == nullptr) {
            continue;
        }
        struct dirent* ent;
        std::string base = d;
        if (!base.empty() && base.back() == '/') {
            base.pop_back();
        }
        while ((ent = ::readdir(dp)) != nullptr) {
            const std::string name = ent->d_name;
            if (name == "." || name == "..") {
                continue;
            }
            if (name.size() >= 6 &&
                (name.compare(name.size() - 6, 6, ".ebook") == 0 ||
                 name.compare(name.size() - 5, 5, ".epub") == 0)) {
                std::string full_path = base + "/" + name;
                if (std::find(out.begin(), out.end(), full_path) == out.end()) {
                    out.push_back(full_path);
                }
            }
        }
        ::closedir(dp);
        if (!out.empty() && dir != nullptr && dir[0] != '\0' && (d == dir || d == std::string(dir) + "/Books")) {
            break;
        }
    }
    std::sort(out.begin(), out.end());
    return out;
}

// Build a single scrollable list row. Returns the object.
static lv_obj_t* BuildRow(lv_obj_t* parent, const char* text, bool selected) {
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, 44);
    lv_obj_set_style_pad_all(row, 8, 0);
    lv_obj_set_style_pad_ver(row, 4, 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(row, 6, 0);
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
    if (selected) {
        lv_obj_set_style_bg_color(row, lv_color_hex(0x2b2b2b), 0);
        lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    }
    lv_obj_t* t = lv_label_create(row);
    lv_label_set_text(t, text);
    lv_label_set_long_mode(t, LV_LABEL_LONG_DOT);
    lv_obj_set_width(t, lv_pct(100));
    lv_obj_set_style_text_color(t, selected ? lv_color_hex(0xF0A500) : lv_color_black(), 0);
    return row;
}

void BuildHomeScreen() {
    auto& ui = Ui();
    auto& sh = Shell();
    lv_obj_t* old_scr = ui.scr;
    ui.content = nullptr;
    ui.footer = nullptr;
    ui.scr = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(ui.scr, lv_color_hex(0xEFEBE0), 0);
    lv_obj_set_style_bg_opa(ui.scr, LV_OPA_COVER, 0);
    lv_obj_set_style_text_font(ui.scr, ListFont(), 0);
    lv_obj_set_style_text_color(ui.scr, lv_color_black(), 0);
    DisableScroll(ui.scr);

    BuildTopStatusBar(ui.scr, "图书库 · Library");

    lv_obj_t* list = lv_obj_create(ui.scr);
    lv_obj_remove_style_all(list);
    lv_obj_set_size(list, EPD_HOR_RES - 24, EPD_VER_RES - 110);
    lv_obj_align(list, LV_ALIGN_TOP_LEFT, 12, ui.status_h + 10);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_ON);

    sh.books = ScanBooks(getenv("EBOOK_LIBDIR") ? getenv("EBOOK_LIBDIR") : "sample");
    if (sh.books.empty()) {
        lv_obj_t* empty = lv_label_create(list);
        lv_label_set_text(empty, "(空)  EBOOK_LIBDIR 未找到 .ebook/.epub");
        lv_obj_set_style_pad_all(empty, 8, 0);
    }
    for (size_t i = 0; i < sh.books.size(); ++i) {
        const std::string& p = sh.books[i];
        const char* nm = p.c_str() + p.find_last_of('/') + 1;
        std::string row_name = nm;
        if (!sh.last_book.empty() && (p == sh.last_book || p.find(sh.last_book) != std::string::npos)) {
            row_name = "[★上次] " + row_name;
        }
        lv_obj_t* row = BuildRow(list, row_name.c_str(), (static_cast<int>(i) == sh.home_sel));
        lv_obj_add_event_cb(
            row,
            [](lv_event_t* e) {
                int idx = static_cast<int>(reinterpret_cast<std::intptr_t>(lv_event_get_user_data(e)));
                auto& s = Shell();
                s.home_sel = idx;
                if (idx >= 0 && idx < static_cast<int>(s.books.size())) {
                    OpenSessionPath(s.books[idx].c_str());
                }
            },
            LV_EVENT_CLICKED,
            reinterpret_cast<void*>(static_cast<std::intptr_t>(i)));
    }

    // Settings shortcut row inside the list.
    const int settings_idx = static_cast<int>(sh.books.size());
    lv_obj_t* set_row = BuildRow(list, "⚙  系统设置 · Settings (S)", sh.home_sel == settings_idx);
    lv_obj_add_event_cb(
        set_row,
        [](lv_event_t*) {
            book_reader_show_settings();
        },
        LV_EVENT_CLICKED,
        nullptr);

    // Standby shortcut row inside the list.
    const int standby_idx = settings_idx + 1;
    lv_obj_t* stb_row = BuildRow(list, "🌙  待机屏保 · Standby (P)", sh.home_sel == standby_idx);
    lv_obj_add_event_cb(
        stb_row,
        [](lv_event_t*) {
            book_reader_enter_standby();
        },
        LV_EVENT_CLICKED,
        nullptr);

    // Settings shortcut / hint at the bottom.
    lv_obj_t* gear = lv_label_create(ui.scr);
    lv_label_set_text(gear, "↑/↓ 选择  ·  ⏎ 打开  ·  P 待机  ·  S 设置  ·  H 返回");
    lv_obj_align(gear, LV_ALIGN_BOTTOM_LEFT, 12, -24);
    lv_obj_set_style_text_font(gear, ListFont(), 0);

    BuildVStrip(ui.scr);
    lv_screen_load(ui.scr);
    if (old_scr != nullptr && lv_obj_is_valid(old_scr)) {
        lv_obj_delete_async(old_scr);
    }
}

void BuildSettingsScreen() {
    auto& ui = Ui();
    auto& sh = Shell();
    lv_obj_t* old_scr = ui.scr;
    ui.content = nullptr;
    ui.footer = nullptr;
    ui.scr = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(ui.scr, lv_color_hex(0xEFEBE0), 0);
    lv_obj_set_style_bg_opa(ui.scr, LV_OPA_COVER, 0);
    lv_obj_set_style_text_font(ui.scr, ListFont(), 0);
    lv_obj_set_style_text_color(ui.scr, lv_color_black(), 0);
    DisableScroll(ui.scr);

    BuildTopStatusBar(ui.scr, "系统设置 · Settings");

    lv_obj_t* list = lv_obj_create(ui.scr);
    lv_obj_remove_style_all(list);
    lv_obj_set_size(list, EPD_HOR_RES - 24, EPD_VER_RES - 110);
    lv_obj_align(list, LV_ALIGN_TOP_LEFT, 12, ui.status_h + 10);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_ON);

    for (int i = 0; i < kFontSizeCount; ++i) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "字号 %d%s", kFontSizes[i],
                      i == sh.font_size_idx ? "  ✓" : "");
        lv_obj_t* row = BuildRow(list, buf, i == sh.settings_sel);
        lv_obj_add_event_cb(
            row,
            [](lv_event_t* e) {
                int idx = static_cast<int>(reinterpret_cast<std::intptr_t>(lv_event_get_user_data(e)));
                auto& s = Shell();
                s.settings_sel = idx;
                s.font_size_idx = idx;
                NvsSaveFontIdx(idx);
                book_reader_set_epdfont_path(FontPathFor(idx).c_str());
                auto& u = Ui();
                if (u.session && u.session->IsOpen()) {
                    s.screen = ShellScreen::kReader;
                } else {
                    BuildSettingsScreen();
                }
            },
            LV_EVENT_CLICKED,
            reinterpret_cast<void*>(static_cast<std::intptr_t>(i)));
    }

    // Refresh mode toggle row
    const int refr_idx = kFontSizeCount;
    char refr_buf[64];
    std::snprintf(refr_buf, sizeof(refr_buf), "刷新模式: %s",
                  sh.refresh_mode == 0 ? "局刷 (Partial) ✓" : "全刷 (Full) ✓");
    lv_obj_t* refr_row = BuildRow(list, refr_buf, sh.settings_sel == refr_idx);
    lv_obj_add_event_cb(
        refr_row,
        [](lv_event_t*) {
            auto& s = Shell();
            s.refresh_mode = 1 - s.refresh_mode;
            NvsSaveRefreshMode(s.refresh_mode);
            BuildSettingsScreen();
        },
        LV_EVENT_CLICKED,
        nullptr);

    // Standby row
    const int standby_idx = kFontSizeCount + 1;
    lv_obj_t* stb_row = BuildRow(list, "🌙  立即进入待机 · Standby (P)", sh.settings_sel == standby_idx);
    lv_obj_add_event_cb(
        stb_row,
        [](lv_event_t*) {
            book_reader_enter_standby();
        },
        LV_EVENT_CLICKED,
        nullptr);

    // Hardware spec card
    lv_obj_t* card = lv_obj_create(list);
    lv_obj_remove_style_all(card);
    lv_obj_set_width(card, lv_pct(100));
    lv_obj_set_style_pad_all(card, 12, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0xBBBBBB), 0);
    lv_obj_set_style_radius(card, 6, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t* info_lbl = lv_label_create(card);
    lv_label_set_text(info_lbl,
        "硬件型号: Metalio E-Ink 4 Plus (684×1216)\n"
        "电量状态: 70% (BQ27220)  ·  时钟: PCF8563\n"
        "按键映射: BOOT(B), VOL+(]/=), VOL-([/-), PWR(P)\n"
        "虚拟触控: HOME(H), PREV(K), NEXT(J)\n"
        "数据存储: NVS File Storage ($ZONHAND_NVS_DIR)");
    lv_obj_set_style_text_font(info_lbl, ListFont(), 0);
    lv_obj_set_style_text_color(info_lbl, lv_color_hex(0x555555), 0);

    lv_obj_t* hint = lv_label_create(ui.scr);
    lv_label_set_text(hint, "↑/↓ 选择  ·  ⏎ 确定/切换  ·  P 待机  ·  ESC/H 返回");
    lv_obj_align(hint, LV_ALIGN_BOTTOM_LEFT, 12, -24);
    lv_obj_set_style_text_font(hint, ListFont(), 0);

    BuildVStrip(ui.scr);
    lv_screen_load(ui.scr);
    if (old_scr != nullptr && lv_obj_is_valid(old_scr)) {
        lv_obj_delete_async(old_scr);
    }
}

void BuildStandbyScreen() {
    auto& ui = Ui();
    lv_obj_t* old_scr = ui.scr;
    ui.content = nullptr;
    ui.footer = nullptr;
    ui.scr = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(ui.scr, lv_color_hex(0x1F2421), 0); // E-paper dark standby wallpaper
    lv_obj_set_style_bg_opa(ui.scr, LV_OPA_COVER, 0);
    lv_obj_set_style_text_font(ui.scr, ListFont(), 0);
    lv_obj_set_style_text_color(ui.scr, lv_color_hex(0xEFEBE0), 0);
    DisableScroll(ui.scr);

    // Frame
    lv_obj_t* card = lv_obj_create(ui.scr);
    lv_obj_remove_style_all(card);
    lv_obj_set_size(card, EPD_HOR_RES - 60, EPD_VER_RES - 200);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x888888), 0);
    lv_obj_set_style_radius(card, 8, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_TRANSP, 0);

    lv_obj_t* brand = lv_label_create(card);
    lv_label_set_text(brand, "Metalio E-Ink 4 Plus");
    lv_obj_align(brand, LV_ALIGN_TOP_MID, 0, 40);

    lv_obj_t* mode = lv_label_create(card);
    lv_label_set_text(mode, "待 机 屏 保 · STANDBY");
    lv_obj_align(mode, LV_ALIGN_TOP_MID, 0, 80);
    lv_obj_set_style_text_color(mode, lv_color_hex(0xAAAAAA), 0);

    lv_obj_t* clock = lv_label_create(card);
    lv_label_set_text(clock, GetTimeString().c_str());
    lv_obj_align(clock, LV_ALIGN_CENTER, 0, -40);

    lv_obj_t* date = lv_label_create(card);
    lv_label_set_text(date, GetDateString().c_str());
    lv_obj_align(date, LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_style_text_color(date, lv_color_hex(0xAAAAAA), 0);

    lv_obj_t* bat = lv_label_create(card);
    std::string bat_str = "电量 " + GetBatteryString();
    lv_label_set_text(bat, bat_str.c_str());
    lv_obj_align(bat, LV_ALIGN_CENTER, 0, 60);

    lv_obj_t* hint = lv_label_create(card);
    lv_label_set_text(hint, "按 P (电源键) 或 任意键唤醒\nPress P (POWER) or Any Key to Wake");
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -40);

    lv_obj_add_event_cb(ui.scr, [](lv_event_t*) {
        book_reader_exit_standby();
    }, LV_EVENT_CLICKED, nullptr);

    BuildVStrip(ui.scr);
    lv_screen_load(ui.scr);
    if (old_scr != nullptr && lv_obj_is_valid(old_scr)) {
        lv_obj_delete_async(old_scr);
    }
}

}  // namespace

extern "C" {

void book_reader_init(void) {
    s_ui_ready = true;
    NvsLoad();
    auto& sh = Shell();
    if (sh.font_dir.empty()) {
        sh.font_dir = ResolveFontDir();
    }
    EnsureEpdfont();
}

void book_reader_show_home(void) {
    auto& sh = Shell();
    sh.screen = ShellScreen::kHome;
    BuildHomeScreen();
}

void book_reader_show_settings(void) {
    auto& sh = Shell();
    sh.prev_screen = sh.screen;
    sh.screen = ShellScreen::kSettings;
    sh.settings_sel = sh.font_size_idx;
    BuildSettingsScreen();
}

void book_reader_toggle_standby(void) {
    auto& sh = Shell();
    if (sh.screen == ShellScreen::kStandby) {
        book_reader_exit_standby();
    } else {
        book_reader_enter_standby();
    }
}

void book_reader_enter_standby(void) {
    auto& sh = Shell();
    if (sh.screen != ShellScreen::kStandby) {
        sh.prev_screen = sh.screen;
        sh.screen = ShellScreen::kStandby;
        BuildStandbyScreen();
    }
}

void book_reader_exit_standby(void) {
    auto& sh = Shell();
    if (sh.screen == ShellScreen::kStandby) {
        auto& ui = Ui();
        if (sh.prev_screen == ShellScreen::kReader && ui.session && ui.session->IsOpen()) {
            sh.screen = ShellScreen::kReader;
            BuildScreen();
            RenderPage();
        } else if (sh.prev_screen == ShellScreen::kSettings) {
            book_reader_show_settings();
        } else {
            book_reader_show_home();
        }
    }
}

bool book_reader_is_standby(void) {
    return Shell().screen == ShellScreen::kStandby;
}

// ---------------------------------------------------------------------------
// Input shim (T4/T2'): wire host mouse + keyboard onto the simulated device's keys.
// Mode-aware: handles navigation on Home, Settings, Standby, and Reader screens.
// ---------------------------------------------------------------------------
void book_reader_input_init(void) {
    if (s_ui_ready == false) return;

    // Pointer: click = next page (already handled by OnContentClicked).
    // Register the SDL mouse indev so clicks register on the body object.
    lv_indev_t * mouse = lv_sdl_mouse_create();
    (void)mouse;

    // Keyboard: catch LV_EVENT_KEY and map mode-aware navigation.
    lv_indev_t * kbd = lv_sdl_keyboard_create();
    if (kbd != nullptr) {
        lv_indev_add_event_cb(
            kbd,
            [](lv_event_t * e) {
                lv_indev_t * indev = (lv_indev_t *)lv_event_get_target(e);
                const lv_event_code_t code = lv_event_get_code(e);
                if (code != LV_EVENT_KEY) {
                    return;
                }
                const uint32_t key = lv_indev_get_key(indev);
                auto& sh = Shell();

                // Standby wake: any key press exits standby
                if (sh.screen == ShellScreen::kStandby) {
                    book_reader_exit_standby();
                    return;
                }

                // Global physical key mappings
                if (key == 'p' || key == 'P') {
                    book_reader_toggle_standby();
                    return;
                }
                if (key == 'b' || key == 'B') {
                    book_reader_show_home();
                    return;
                }

                if (sh.screen == ShellScreen::kHome) {
                    const int max_sel = static_cast<int>(sh.books.size()) + 1;
                    if (key == LV_KEY_UP || key == LV_KEY_LEFT || key == 'k' || key == 'K' || key == 'a' || key == 'A' || key == '=' || key == '+' || key == ']') {
                        if (sh.home_sel > 0) {
                            sh.home_sel--;
                            BuildHomeScreen();
                        }
                    } else if (key == LV_KEY_DOWN || key == LV_KEY_RIGHT || key == 'j' || key == 'J' || key == 'd' || key == 'D' || key == '-' || key == '_' || key == '[') {
                        if (sh.home_sel < max_sel) {
                            sh.home_sel++;
                            BuildHomeScreen();
                        }
                    } else if (key == LV_KEY_ENTER || key == '\n' || key == '\r') {
                        if (sh.home_sel < static_cast<int>(sh.books.size())) {
                            OpenSessionPath(sh.books[sh.home_sel].c_str());
                        } else if (sh.home_sel == static_cast<int>(sh.books.size())) {
                            book_reader_show_settings();
                        } else {
                            book_reader_enter_standby();
                        }
                    } else if (key == 's' || key == 'S') {
                        book_reader_show_settings();
                    } else if (key == 'h' || key == 'H' || key == LV_KEY_ESC) {
                        sh.home_sel = 0;
                        BuildHomeScreen();
                    }
                } else if (sh.screen == ShellScreen::kSettings) {
                    const int max_settings_sel = kFontSizeCount + 1;
                    if (key == LV_KEY_UP || key == LV_KEY_LEFT || key == 'k' || key == 'K' || key == 'a' || key == 'A' || key == '=' || key == '+' || key == ']') {
                        if (sh.settings_sel > 0) {
                            sh.settings_sel--;
                            BuildSettingsScreen();
                        }
                    } else if (key == LV_KEY_DOWN || key == LV_KEY_RIGHT || key == 'j' || key == 'J' || key == 'd' || key == 'D' || key == '-' || key == '_' || key == '[') {
                        if (sh.settings_sel < max_settings_sel) {
                            sh.settings_sel++;
                            BuildSettingsScreen();
                        }
                    } else if (key == LV_KEY_ENTER || key == '\n' || key == '\r') {
                        if (sh.settings_sel < kFontSizeCount) {
                            sh.font_size_idx = sh.settings_sel;
                            NvsSaveFontIdx(sh.font_size_idx);
                            std::string fp = FontPathFor(sh.font_size_idx);
                            book_reader_set_epdfont_path(fp.c_str());
                            auto& u = Ui();
                            if (u.session && u.session->IsOpen()) {
                                sh.screen = ShellScreen::kReader;
                            } else {
                                BuildSettingsScreen();
                            }
                        } else if (sh.settings_sel == kFontSizeCount) {
                            sh.refresh_mode = 1 - sh.refresh_mode;
                            NvsSaveRefreshMode(sh.refresh_mode);
                            BuildSettingsScreen();
                        } else {
                            book_reader_enter_standby();
                        }
                    } else if (key == LV_KEY_ESC || key == 'h' || key == 'H' || key == 'q' || key == 'Q') {
                        auto& u = Ui();
                        if (sh.prev_screen == ShellScreen::kReader && u.session && u.session->IsOpen()) {
                            sh.screen = ShellScreen::kReader;
                            BuildScreen();
                            RenderPage();
                        } else {
                            book_reader_show_home();
                        }
                    }
                } else {  // ShellScreen::kReader
                    if (key == LV_KEY_LEFT || key == LV_KEY_UP || key == LV_KEY_PREV || key == 'a' || key == 'A' || key == 'k' || key == 'K' || key == '=' || key == '+' || key == ']') {
                        book_reader_prev_page();
                    } else if (key == LV_KEY_RIGHT || key == LV_KEY_DOWN || key == LV_KEY_NEXT || key == 'd' || key == 'D' || key == 'j' || key == 'J' || key == '-' || key == '_' || key == '[') {
                        book_reader_next_page();
                    } else if (key == 'h' || key == 'H') {
                        book_reader_vk_home();
                    } else if (key == 's' || key == 'S') {
                        sh.prev_screen = ShellScreen::kReader;
                        book_reader_show_settings();
                    } else if (key == LV_KEY_ESC || key == 'q' || key == 'Q') {
                        book_reader_show_home();
                    }
                }
            },
            LV_EVENT_ALL,
            nullptr);
    }
}

void book_reader_set_fontpack_path(const char* /*path*/) {
    /* 模拟器/Web 仅使用 .ef，不再加载 fontpack */
}

void book_reader_set_epdfont_path(const char* path) {
    auto& ui = Ui();
    auto& sh = Shell();
    const bool had_book = ui.session && ui.session->IsOpen();
    int saved_toc = -1;
    int saved_page = 0;
    std::string reopen_path;
    if (had_book) {
        saved_toc = ui.session->CurrentTocIndex();
        saved_page = ui.session->CurrentPage();
        reopen_path = ui.open_ebook_path;
    }

    ReleaseEpdfont();
    ui.epdfont_path = path != nullptr ? path : "";
    epdfont_emulator_set_path(path);
    if (!s_ui_ready) {
        return;
    }

    // 换字体须重新打开并分页，否则 viewport/页表仍是旧字模，正文会压住底栏
    if (had_book && !reopen_path.empty()) {
        EnsureEpdfont();
        if (!OpenSessionPath(reopen_path.c_str())) {
            ShowIdleScreen();
            return;
        }
        if (saved_toc >= 0) {
            ui.session->GoToToc(saved_toc);
        }
        if (saved_page > 0) {
            ui.session->GoToPage(saved_page);
        }
        RenderPage();
        return;
    }

    if (ui.session && ui.session->IsOpen()) {
        EnsureEpdfont();
        ui.session->SetFont(BookFont());
        BuildScreen();
        RenderPage();
    } else {
        if (sh.screen == ShellScreen::kHome) {
            BuildHomeScreen();
        } else if (sh.screen == ShellScreen::kSettings) {
            BuildSettingsScreen();
        } else {
            ShowIdleScreen();
        }
    }
}

int book_reader_open_ebook_path(const char* path) {
    return OpenSessionPath(path) ? 1 : 0;
}

int book_reader_open_ebook_mem(const uint8_t* data, size_t len) {
    auto& ui = Ui();
    if (data == nullptr || len == 0) {
        ui.last_error = "空数据";
        return 0;
    }
    char tmp_path[64];
    std::snprintf(tmp_path, sizeof(tmp_path), "/tmp/ebook_%p.ebook", static_cast<const void*>(data));
#ifdef __EMSCRIPTEN__
    std::snprintf(tmp_path, sizeof(tmp_path), "/ebook_open.ebook");
#endif
    FILE* fp = fopen(tmp_path, "wb");
    if (fp == nullptr) {
        ui.last_error = "无法写入临时文件";
        return 0;
    }
    if (fwrite(data, 1, len, fp) != len) {
        fclose(fp);
        ui.last_error = "写入临时文件失败";
        return 0;
    }
    fclose(fp);
    ui.temp_ebook_path = tmp_path;
    return OpenSessionPath(tmp_path) ? 1 : 0;
}

void book_reader_next_page(void) {
    auto& ui = Ui();
    if (ui.session && ui.session->NextPage()) {
        RenderPage();
    }
}

void book_reader_vk_home(void) {
    auto& ui = Ui();
    auto& sh = Shell();
    if (sh.screen == ShellScreen::kReader) {
        if (ui.session && ui.session->IsOpen()) {
            if (ui.session->CurrentPage() > 0) {
                ui.session->GoToPage(0);
                RenderPage();
                return;
            }
        }
    }
    book_reader_show_home();
}

void book_reader_prev_page(void) {
    auto& ui = Ui();
    if (ui.session && ui.session->PrevPage()) {
        RenderPage();
    }
}

void book_reader_go_toc(int index) {
    auto& ui = Ui();
    if (ui.session && ui.session->GoToToc(index)) {
        RenderPage();
    }
}

int book_reader_toc_count(void) {
    auto& ui = Ui();
    return ui.session ? ui.session->TocCount() : 0;
}

const char* book_reader_toc_title(int index) {
    static std::string s_toc_title;
    auto& ui = Ui();
    s_toc_title.clear();
    if (!ui.session || index < 0) {
        return "";
    }
    const reader::TocEntry* ent = ui.session->TocAt(index);
    if (ent == nullptr) {
        return "";
    }
    s_toc_title = ent->title;
    return s_toc_title.c_str();
}

int book_reader_current_toc_index(void) {
    auto& ui = Ui();
    return ui.session ? ui.session->CurrentTocIndex() : -1;
}

const char* book_reader_last_error(void) {
    return Ui().last_error.c_str();
}

}  // extern "C"
