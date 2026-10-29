#include "epdiy.h"
#include "epd_highlevel.h"
#include "display.h"
#include "display_main_waveform.h"
#include "display_reader_waveform.h"
#include "metalio_text_ripple.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

#define DISPLAY_WIDTH 1216
#define DISPLAY_HEIGHT 684
#define ROTATED_WIDTH 684
#define ROTATED_HEIGHT 1216

static enum EpdRotation s_rotation = EPD_ROT_PORTRAIT;
static bool s_dirty = true;
static uint8_t *s_active_fb = NULL;

const EpdWaveform E0470_WAVEFORM = {0};
const EpdWaveform E0470_FOLLOW_WAVEFORM = {0};
const EpdWaveform E0470_FULL_WAVEFORM = {0};

const EpdWaveform* display_main_gray_waveform(void) { return &E0470_WAVEFORM; }
const EpdWaveform* display_main_bw_waveform(void) { return &E0470_WAVEFORM; }
const EpdWaveform* display_main_normal_waveform(void) { return &E0470_WAVEFORM; }
const EpdWaveform* display_main_water_waveform(void) { return &E0470_WAVEFORM; }
const EpdWaveform* display_reader_text_waveform(void) { return &E0470_FULL_WAVEFORM; }

int epd_width(void) { return DISPLAY_WIDTH; }
int epd_height(void) { return DISPLAY_HEIGHT; }
int epd_rotated_display_width(void) { return ROTATED_WIDTH; }
int epd_rotated_display_height(void) { return ROTATED_HEIGHT; }

void epd_set_rotation(enum EpdRotation rotation) { s_rotation = rotation; }
enum EpdRotation epd_get_rotation(void) { return s_rotation; }

EpdRect epd_full_screen(void) {
    return (EpdRect){ .x = 0, .y = 0, .width = ROTATED_WIDTH, .height = ROTATED_HEIGHT };
}

void epd_clear(void) {
    if (s_active_fb) {
        memset(s_active_fb, 0xFF, (DISPLAY_WIDTH * DISPLAY_HEIGHT) / 2);
        s_dirty = true;
    }
}

void epd_clear_area(EpdRect area) {
    if (s_active_fb) {
        epd_fill_rect(area, 255, s_active_fb);
    }
}

void epd_poweron(void) {}
void epd_poweroff(void) {}
void epd_set_vcom(uint16_t vcom) { (void)vcom; }
float epd_ambient_temperature(void) { return 25.0f; }

void epd_hl_set_all_white(EpdiyHighlevelState *hl) { (void)hl; epd_clear(); }
void epd_hl_set_all_black(EpdiyHighlevelState *hl) {
    (void)hl;
    if (s_active_fb) {
        memset(s_active_fb, 0x00, (DISPLAY_WIDTH * DISPLAY_HEIGHT) / 2);
        s_dirty = true;
    }
}

void sim_display_set_framebuffer(uint8_t *fb) {
    s_active_fb = fb;
}

uint8_t* sim_display_get_framebuffer(void) {
    return s_active_fb;
}

bool sim_display_is_dirty(void) {
    return s_dirty;
}

void sim_display_clear_dirty(void) {
    s_dirty = false;
}

void sim_display_mark_dirty(void) {
    s_dirty = true;
}

/* 4bpp pixel drawing implementation matching epdiy.c */
void epd_draw_pixel(int x, int y, uint8_t color, uint8_t* framebuffer) {
    if (!framebuffer) framebuffer = s_active_fb;
    if (!framebuffer) return;

    int px = x, py = y;
    switch (s_rotation) {
        case EPD_ROT_PORTRAIT: px = DISPLAY_WIDTH - y - 1; py = x; break;
        case EPD_ROT_INVERTED_LANDSCAPE: px = DISPLAY_WIDTH - x - 1; py = DISPLAY_HEIGHT - y - 1; break;
        case EPD_ROT_INVERTED_PORTRAIT: px = y; py = DISPLAY_HEIGHT - x - 1; break;
        default: break;
    }
    if ((unsigned)px >= (unsigned)DISPLAY_WIDTH || (unsigned)py >= (unsigned)DISPLAY_HEIGHT) return;
    uint8_t *out = framebuffer + py * (DISPLAY_WIDTH / 2) + px / 2;
    if (px & 1) *out = (*out & 15) | (color & 240);
    else *out = (*out & 240) | (color >> 4);
    s_dirty = true;
}

uint8_t epd_get_pixel(int x, int y, int fb_width, int fb_height, const uint8_t* framebuffer) {
    if (!framebuffer) framebuffer = s_active_fb;
    if (!framebuffer || x < 0 || x >= fb_width || y < 0 || y >= fb_height) return 0;
    int fb_width_bytes = fb_width / 2 + fb_width % 2;
    uint8_t buf_val = framebuffer[y * fb_width_bytes + x / 2];
    if (x & 1) buf_val = (buf_val & 0xF0) >> 4;
    else buf_val = (buf_val & 0x0F);
    return buf_val << 4;
}

void epd_draw_hline(int x, int y, int length, uint8_t color, uint8_t* framebuffer) {
    for (int i = 0; i < length; i++) {
        epd_draw_pixel(x + i, y, color, framebuffer);
    }
}

void epd_draw_vline(int x, int y, int length, uint8_t color, uint8_t* framebuffer) {
    for (int i = 0; i < length; i++) {
        epd_draw_pixel(x, y + i, color, framebuffer);
    }
}

void epd_draw_rect(EpdRect rect, uint8_t color, uint8_t* framebuffer) {
    epd_draw_hline(rect.x, rect.y, rect.width, color, framebuffer);
    epd_draw_hline(rect.x, rect.y + rect.height - 1, rect.width, color, framebuffer);
    epd_draw_vline(rect.x, rect.y, rect.height, color, framebuffer);
    epd_draw_vline(rect.x + rect.width - 1, rect.y, rect.height, color, framebuffer);
}

void epd_fill_rect(EpdRect rect, uint8_t color, uint8_t* framebuffer) {
    for (int y = rect.y; y < rect.y + rect.height; y++) {
        epd_draw_hline(rect.x, y, rect.width, color, framebuffer);
    }
}

static void swap_int(int *a, int *b) {
    int t = *a; *a = *b; *b = t;
}

void epd_draw_line(int x0, int y0, int x1, int y1, uint8_t color, uint8_t* framebuffer) {
    if (x0 == x1) {
        if (y0 > y1) swap_int(&y0, &y1);
        epd_draw_vline(x0, y0, y1 - y0 + 1, color, framebuffer);
        return;
    }
    if (y0 == y1) {
        if (x0 > x1) swap_int(&x0, &x1);
        epd_draw_hline(x0, y0, x1 - x0 + 1, color, framebuffer);
        return;
    }
    int steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep) { swap_int(&x0, &y0); swap_int(&x1, &y1); }
    if (x0 > x1) { swap_int(&x0, &x1); swap_int(&y0, &y1); }
    int dx = x1 - x0;
    int dy = abs(y1 - y0);
    int err = dx / 2;
    int ystep = (y0 < y1) ? 1 : -1;
    for (; x0 <= x1; x0++) {
        if (steep) epd_draw_pixel(y0, x0, color, framebuffer);
        else epd_draw_pixel(x0, y0, color, framebuffer);
        err -= dy;
        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
    }
}

void epd_draw_circle(int x0, int y0, int r, uint8_t color, uint8_t* framebuffer) {
    int f = 1 - r;
    int ddF_x = 1, ddF_y = -2 * r;
    int x = 0, y = r;
    epd_draw_pixel(x0, y0 + r, color, framebuffer);
    epd_draw_pixel(x0, y0 - r, color, framebuffer);
    epd_draw_pixel(x0 + r, y0, color, framebuffer);
    epd_draw_pixel(x0 - r, y0, color, framebuffer);
    while (x < y) {
        if (f >= 0) { y--; ddF_y += 2; f += ddF_y; }
        x++; ddF_x += 2; f += ddF_x;
        epd_draw_pixel(x0 + x, y0 + y, color, framebuffer);
        epd_draw_pixel(x0 - x, y0 + y, color, framebuffer);
        epd_draw_pixel(x0 + x, y0 - y, color, framebuffer);
        epd_draw_pixel(x0 - x, y0 - y, color, framebuffer);
        epd_draw_pixel(x0 + y, y0 + x, color, framebuffer);
        epd_draw_pixel(x0 - y, y0 + x, color, framebuffer);
        epd_draw_pixel(x0 + y, y0 - x, color, framebuffer);
        epd_draw_pixel(x0 - y, y0 - x, color, framebuffer);
    }
}

void epd_fill_circle_helper(int x0, int y0, int r, int corners, int delta, uint8_t color, uint8_t* framebuffer) {
    int f = 1 - r;
    int ddF_x = 1, ddF_y = -2 * r;
    int x = 0, y = r, px = x, py = y;
    delta++;
    while (x < y) {
        if (f >= 0) { y--; ddF_y += 2; f += ddF_y; }
        x++; ddF_x += 2; f += ddF_x;
        if (x < (y + 1)) {
            if (corners & 1) epd_draw_vline(x0 + x, y0 - y, 2 * y + delta, color, framebuffer);
            if (corners & 2) epd_draw_vline(x0 - x, y0 - y, 2 * y + delta, color, framebuffer);
        }
        if (y != py) {
            if (corners & 1) epd_draw_vline(x0 + py, y0 - px, 2 * px + delta, color, framebuffer);
            if (corners & 2) epd_draw_vline(x0 - py, y0 - px, 2 * px + delta, color, framebuffer);
            py = y;
        }
        px = x;
    }
}

void epd_fill_circle(int x0, int y0, int r, uint8_t color, uint8_t* framebuffer) {
    epd_draw_vline(x0, y0 - r, 2 * r + 1, color, framebuffer);
    epd_fill_circle_helper(x0, y0, r, 3, 0, color, framebuffer);
}

void epd_draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t color, uint8_t* framebuffer) {
    epd_draw_line(x0, y0, x1, y1, color, framebuffer);
    epd_draw_line(x1, y1, x2, y2, color, framebuffer);
    epd_draw_line(x2, y2, x0, y0, color, framebuffer);
}

void epd_fill_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t color, uint8_t* framebuffer) {
    int a, b, y, last;
    if (y0 > y1) { swap_int(&y0, &y1); swap_int(&x0, &x1); }
    if (y1 > y2) { swap_int(&y2, &y1); swap_int(&x2, &x1); }
    if (y0 > y1) { swap_int(&y0, &y1); swap_int(&x0, &x1); }
    if (y0 == y2) {
        a = b = x0;
        if (x1 < a) a = x1; else if (x1 > b) b = x1;
        if (x2 < a) a = x2; else if (x2 > b) b = x2;
        epd_draw_hline(a, y0, b - a + 1, color, framebuffer);
        return;
    }
    int dx01 = x1 - x0, dy01 = y1 - y0, dx02 = x2 - x0, dy02 = y2 - y0, dx12 = x2 - x1, dy12 = y2 - y1;
    int32_t sa = 0, sb = 0;
    if (y1 == y2) last = y1; else last = y1 - 1;
    for (y = y0; y <= last; y++) {
        a = x0 + sa / dy01;
        b = x0 + sb / dy02;
        sa += dx01; sb += dx02;
        if (a > b) swap_int(&a, &b);
        epd_draw_hline(a, y, b - a + 1, color, framebuffer);
    }
    sa = (int32_t)dx12 * (y - y1);
    sb = (int32_t)dx02 * (y - y0);
    for (; y <= y2; y++) {
        a = x1 + sa / dy12;
        b = x0 + sb / dy02;
        sa += dx12; sb += dx02;
        if (a > b) swap_int(&a, &b);
        epd_draw_hline(a, y, b - a + 1, color, framebuffer);
    }
}

void epd_copy_to_framebuffer(EpdRect image_area, const uint8_t* image_buffer, uint8_t* framebuffer) {
    if (!framebuffer) framebuffer = s_active_fb;
    for (int y = 0; y < image_area.height; y++) {
        for (int x = 0; x < image_area.width; x++) {
            uint8_t c = epd_get_pixel(x, y, image_area.width, image_area.height, image_buffer);
            epd_draw_pixel(image_area.x + x, image_area.y + y, c, framebuffer);
        }
    }
}

void epd_draw_rotated_transparent_image(EpdRect image_area, const uint8_t* image_buffer, uint8_t* framebuffer, uint8_t transparent_color) {
    if (!framebuffer) framebuffer = s_active_fb;
    for (int y = 0; y < image_area.height; y++) {
        for (int x = 0; x < image_area.width; x++) {
            uint8_t c = epd_get_pixel(x, y, image_area.width, image_area.height, image_buffer);
            if (c != transparent_color) {
                epd_draw_pixel(image_area.x + x, image_area.y + y, c, framebuffer);
            }
        }
    }
}

void epd_draw_rotated_image(EpdRect image_area, const uint8_t* image_buffer, uint8_t* framebuffer) {
    epd_copy_to_framebuffer(image_area, image_buffer, framebuffer);
}

/* display.h functions */
enum EpdDrawError update_display_fast_page(EpdiyHighlevelState* hl) { (void)hl; s_dirty = true; return EPD_DRAW_SUCCESS; }
enum EpdDrawError update_display_mode(EpdiyHighlevelState* hl, enum EpdDrawMode mode) { (void)hl; (void)mode; s_dirty = true; return EPD_DRAW_SUCCESS; }
enum EpdDrawError update_display_mode_diff(EpdiyHighlevelState* hl, enum EpdDrawMode mode) { (void)hl; (void)mode; s_dirty = true; return EPD_DRAW_SUCCESS; }
enum EpdDrawError update_display_from_white(EpdiyHighlevelState* hl) { (void)hl; s_dirty = true; return EPD_DRAW_SUCCESS; }
enum EpdDrawError update_display_from_white_with(EpdiyHighlevelState* hl, const EpdWaveform* waveform, enum EpdDrawMode mode) {
    (void)hl; (void)waveform; (void)mode; s_dirty = true; return EPD_DRAW_SUCCESS;
}
enum EpdDrawError update_display_white(EpdiyHighlevelState* hl) { (void)hl; s_dirty = true; return EPD_DRAW_SUCCESS; }
enum EpdDrawError update_display_full(EpdiyHighlevelState* hl) { (void)hl; s_dirty = true; return EPD_DRAW_SUCCESS; }
enum EpdDrawError update_display_image_gray(EpdiyHighlevelState* hl) { (void)hl; s_dirty = true; return EPD_DRAW_SUCCESS; }
enum EpdDrawError update_display_with(EpdiyHighlevelState* hl, const EpdWaveform* waveform, enum EpdDrawMode mode) {
    (void)hl; (void)waveform; (void)mode; s_dirty = true; return EPD_DRAW_SUCCESS;
}
enum EpdDrawError update_display_area_with(EpdiyHighlevelState* hl, const EpdWaveform* waveform, enum EpdDrawMode mode, EpdRect area) {
    (void)hl; (void)waveform; (void)mode; (void)area; s_dirty = true; return EPD_DRAW_SUCCESS;
}
enum EpdDrawError update_display_click_feedback(EpdiyHighlevelState* hl, EpdRect area) {
    (void)hl; (void)area; s_dirty = true; return EPD_DRAW_SUCCESS;
}
enum EpdDrawError update_display_area_full_with(EpdiyHighlevelState* hl, const EpdWaveform* waveform, enum EpdDrawMode mode, EpdRect area) {
    (void)hl; (void)waveform; (void)mode; (void)area; s_dirty = true; return EPD_DRAW_SUCCESS;
}
enum EpdDrawError update_display_area_diff_with(EpdiyHighlevelState* hl, const EpdWaveform* waveform, enum EpdDrawMode mode, EpdRect area) {
    (void)hl; (void)waveform; (void)mode; (void)area; s_dirty = true; return EPD_DRAW_SUCCESS;
}
enum EpdDrawError update_display_water_turn(EpdiyHighlevelState* hl, EpdRect area, e0470_turn_dir_t dir) {
    (void)hl; (void)area; (void)dir; s_dirty = true; return EPD_DRAW_SUCCESS;
}
enum EpdDrawError update_display_text_turn(EpdiyHighlevelState* hl, EpdRect area, e0470_turn_dir_t dir) {
    (void)hl; (void)area; (void)dir; s_dirty = true; return EPD_DRAW_SUCCESS;
}
enum EpdDrawError update_display_main_turn(EpdiyHighlevelState* hl, EpdRect area, e0470_turn_dir_t dir) {
    (void)hl; (void)area; (void)dir; s_dirty = true; return EPD_DRAW_SUCCESS;
}
enum EpdDrawError update_display_gray_transition(EpdiyHighlevelState* hl, EpdRect area) {
    (void)hl; (void)area; s_dirty = true; return EPD_DRAW_SUCCESS;
}
enum EpdDrawError update_display_reader_panel(EpdiyHighlevelState* hl, EpdRect area) {
    (void)hl; (void)area; s_dirty = true; return EPD_DRAW_SUCCESS;
}
enum EpdDrawError update_display_reader_panel_fast(EpdiyHighlevelState* hl, EpdRect area) {
    (void)hl; (void)area; s_dirty = true; return EPD_DRAW_SUCCESS;
}
enum EpdDrawError update_display_reader_panel_gray4(EpdiyHighlevelState* hl, EpdRect area, bool erase_underlay) {
    (void)hl; (void)area; (void)erase_underlay; s_dirty = true; return EPD_DRAW_SUCCESS;
}

// metalio_text_ripple stubs
bool metalio_text_ripple_prepare(EpdiyHighlevelState* hl, EpdRect area) { (void)hl; (void)area; return false; }
const EpdWaveform* metalio_text_ripple_waveform(void) { return NULL; }
void metalio_text_ripple_disable(void) {}

void display_request_full_refresh(void) { s_dirty = true; }
void display_main_transition_arm(const void* owner, bool leaving_shelf, bool changing_page) {
    (void)owner; (void)leaving_shelf; (void)changing_page;
}
void display_main_transition_disarm(void) {}
void display_main_transition_cancel(void) {}
void display_main_transition_gray_bands(const EpdRect* bands, unsigned count) { (void)bands; (void)count; }
void display_main_transition_monochrome(void) {}
void display_main_transition_shelf_page(void) {}
void display_monochrome_content(uint8_t* fb) { (void)fb; }
void display_monochrome_frame(uint8_t* fb) { (void)fb; }

static bool s_system_binary = false;
static bool s_system_navigation = false;
static bool s_system_fast = false;

bool display_system_binary_set(bool enabled) { bool o = s_system_binary; s_system_binary = enabled; return o; }
bool display_system_navigation_set(bool visible) { bool o = s_system_navigation; s_system_navigation = visible; return o; }
bool display_system_fast_set(bool enabled) { bool o = s_system_fast; s_system_fast = enabled; return o; }
bool display_main_switch_erase_set(bool active) { (void)active; return false; }
bool display_main_shelf_home_erase_set(bool active) { (void)active; return false; }

bool display_page_prepare_begin(void) { return false; }
void display_page_prepare_end(bool held) { (void)held; }
bool display_pipeline_enabled(void) { return true; }

void rails_keepalive(void) {}
void rails_idle_check(int64_t now_ms) { (void)now_ms; }
void display_set_bulk_io(bool active) { (void)active; }

bool pico_online_display_begin(void) { return false; }
void pico_online_display_end(bool held) { (void)held; }

uint8_t* epd_hl_get_framebuffer(EpdiyHighlevelState* hl) {
    (void)hl;
    return s_active_fb;
}

void guard_draw_result(EpdiyHighlevelState* hl, enum EpdDrawError result) {
    (void)hl; (void)result;
}

enum EpdDrawError display_panel_repair(EpdiyHighlevelState* hl) {
    (void)hl;
    s_dirty = true;
    return EPD_DRAW_SUCCESS;
}

bool display_reader_fast_panels_enabled(void) {
    return false;
}

bool display_reader_panel_gray4_prepare(uint8_t* target, EpdRect area) {
    (void)target; (void)area;
    return false;
}

bool display_take_white_exit(void) { return false; }

void display_main_transition_cover_bands(const EpdRect* bands, unsigned count) {
    (void)bands; (void)count;
}

void e0470_page_turn_release(void) {}
void e0470_page_turn_set_tick_us(int us) { (void)us; }
int e0470_page_turn_tick_us(void) { return 1000; }
