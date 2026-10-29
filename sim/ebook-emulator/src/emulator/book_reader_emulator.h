#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/** GDEM0397T81P 竖屏逻辑分辨率（与固件 ROTATE_270 后 LV_HOR×LV_VER 一致） */
#define EPD_HOR_RES 684
#define EPD_VER_RES 1216

/** T5: virtual-key strip — the 3 out-of-band touch marks on the device's right
 *  bezel (FT6336 Y=1600, X=100/200/300 -> vk_home/vk_prev/vk_next). Rendered
 *  OUTSIDE the e-paper region so the reader screen keeps its true 684x1216. */
#define VSTRIP_W 44
#define EPD_SIM_VIEW_W (EPD_HOR_RES + VSTRIP_W)

void book_reader_init(void);
/* Register host input (SDL mouse pointer + keyboard) so the simulated
 * device's physical/virtual keys map onto page actions. Call after
 * book_reader_init() and after the display backend is up. */
void book_reader_input_init(void);
void book_reader_set_fontpack_path(const char* path);
void book_reader_set_epdfont_path(const char* path);
int book_reader_open_ebook_path(const char* path);
int book_reader_open_ebook_mem(const uint8_t* data, size_t len);
void book_reader_next_page(void);
void book_reader_prev_page(void);
/** vk_home (sim semantic): back to the book's first page. The real device's
 *  vk_home "one-tap back to system home" needs the app shell (T2+); inside the
 *  reader the closest host behavior is page 0. */
void book_reader_vk_home(void);

/* T2' host app shell: Home (book library) + Settings (font size), layered on
 * top of the reader core. Suggested boot flow:
 *   book_reader_init() -> book_reader_input_init() -> [EBOOK_PATH? open book]
 * Home shows the library (EBOOK_LIBDIR), ENTER opens the selected .ebook,
 * the settings row (or 'S'-free: last row) opens font-size settings. */
void book_reader_show_home(void);
void book_reader_show_settings(void);
void book_reader_go_toc(int index);
int book_reader_toc_count(void);
const char* book_reader_toc_title(int index);
int book_reader_current_toc_index(void);
const char* book_reader_last_error(void);

#ifdef __cplusplus
}
#endif
