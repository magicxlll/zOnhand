#include <SDL.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include "cst836u.h"
#include "metalio_hw.h"
#include "sim_vfs.h"
#include "epdiy.h"
#include "epd_highlevel.h"
#include "app_loop.h"
#include "app_registry.h"
#include "settings.h"
#include "ttf_font.h"
#include "app_font_context.h"
#include "boot_state.h"

#define SCREEN_W 684
#define SCREEN_H 1216
#define BEZEL_W  116
#define TOTAL_W  (SCREEN_W + BEZEL_W)
#define TOTAL_H  SCREEN_H

// Initial window size on desktop
#define WIN_INIT_W 500
#define WIN_INIT_H 760

static SDL_Window *s_window = NULL;
static SDL_Renderer *s_renderer = NULL;
static SDL_Texture *s_screen_texture = NULL;
static uint32_t s_pixels[SCREEN_W * SCREEN_H];
static bool s_running = true;
static bool s_mouse_down = false;

// Virtual bezel buttons
typedef struct {
    SDL_Rect rect;
    const char *label;
    const char *sublabel;
    int key_code;
    uint32_t flash_until;
} bezel_btn_t;

static bezel_btn_t s_buttons[] = {
    { { SCREEN_W + 12, 100, 92, 56 }, "POWER", "Lock/Esc", 1, 0 },
    { { SCREEN_W + 12, 200, 92, 64 }, "VOL +", "Prev/Up",  2, 0 },
    { { SCREEN_W + 12, 280, 92, 64 }, "VOL -", "Next/Dn",  3, 0 },
    { { SCREEN_W + 12, 380, 92, 64 }, "AI KEY", "Menu/A",  4, 0 },
};
#define NUM_BUTTONS (sizeof(s_buttons) / sizeof(s_buttons[0]))

// External display functions
void sim_display_set_framebuffer(uint8_t *fb);
uint8_t* sim_display_get_framebuffer(void);
bool sim_display_is_dirty(void);
void sim_display_clear_dirty(void);
void sim_display_mark_dirty(void);

static void render_bezel(void) {
    // Bezel background
    SDL_SetRenderDrawColor(s_renderer, 32, 35, 40, 255);
    SDL_Rect bezel_rect = { SCREEN_W, 0, BEZEL_W, TOTAL_H };
    SDL_RenderFillRect(s_renderer, &bezel_rect);

    // Bezel divider line
    SDL_SetRenderDrawColor(s_renderer, 60, 64, 72, 255);
    SDL_RenderDrawLine(s_renderer, SCREEN_W, 0, SCREEN_W, TOTAL_H);

    // Brand title header
    SDL_SetRenderDrawColor(s_renderer, 200, 200, 200, 255);
    // Draw decorative indicator at top of bezel
    SDL_Rect top_bar = { SCREEN_W + 20, 30, BEZEL_W - 40, 4 };
    SDL_RenderFillRect(s_renderer, &top_bar);

    uint32_t now = SDL_GetTicks();

    // Render virtual buttons
    for (size_t i = 0; i < NUM_BUTTONS; i++) {
        bezel_btn_t *b = &s_buttons[i];
        if (now < b->flash_until) {
            SDL_SetRenderDrawColor(s_renderer, 220, 140, 40, 255); // orange active flash
        } else {
            SDL_SetRenderDrawColor(s_renderer, 50, 54, 62, 255); // button fill
        }
        SDL_RenderFillRect(s_renderer, &b->rect);

        // Button outline
        SDL_SetRenderDrawColor(s_renderer, 85, 90, 100, 255);
        SDL_RenderDrawRect(s_renderer, &b->rect);

        // Highlight line on top of button
        SDL_SetRenderDrawColor(s_renderer, 110, 115, 125, 255);
        SDL_RenderDrawLine(s_renderer, b->rect.x + 2, b->rect.y + 2, b->rect.x + b->rect.w - 3, b->rect.y + 2);

        // Small indicator dot inside button
        SDL_Rect dot = { b->rect.x + b->rect.w / 2 - 4, b->rect.y + 12, 8, 8 };
        if (i == 0) SDL_SetRenderDrawColor(s_renderer, 220, 80, 80, 255); // power red
        else if (i == 3) SDL_SetRenderDrawColor(s_renderer, 80, 160, 240, 255); // AI blue
        else SDL_SetRenderDrawColor(s_renderer, 180, 180, 180, 255); // vol gray
        SDL_RenderFillRect(s_renderer, &dot);
    }

    // Bottom info area
    SDL_Rect bottom_box = { SCREEN_W + 10, TOTAL_H - 120, BEZEL_W - 20, 100 };
    SDL_SetRenderDrawColor(s_renderer, 40, 44, 50, 255);
    SDL_RenderFillRect(s_renderer, &bottom_box);
    SDL_SetRenderDrawColor(s_renderer, 70, 75, 85, 255);
    SDL_RenderDrawRect(s_renderer, &bottom_box);
}

static void sim_render_screen(void) {
    uint8_t *fb = sim_display_get_framebuffer();
    if (!fb || !s_screen_texture || !s_renderer) return;

    // Convert 4bpp rotated framebuffer to 32bpp ARGB8888
    // logical (x, y): x in [0..SCREEN_W-1], y in [0..SCREEN_H-1]
    // Under EPD_ROT_PORTRAIT: px = 1216 - y - 1, py = x
    for (int y = 0; y < SCREEN_H; y++) {
        int px = 1216 - y - 1;
        int row_idx = y * SCREEN_W;
        for (int x = 0; x < SCREEN_W; x++) {
            int py = x;
            int byte_idx = py * 608 + px / 2;
            uint8_t b = fb[byte_idx];
            uint8_t val = (px & 1) ? (b >> 4) : (b & 0x0F);
            uint8_t gray = val * 17; // 0..15 -> 0..255
            s_pixels[row_idx + x] = 0xFF000000 | (gray << 16) | (gray << 8) | gray;
        }
    }

    SDL_UpdateTexture(s_screen_texture, NULL, s_pixels, SCREEN_W * sizeof(uint32_t));

    SDL_Rect dst_rect = { 0, 0, SCREEN_W, SCREEN_H };
    SDL_RenderCopy(s_renderer, s_screen_texture, NULL, &dst_rect);

    render_bezel();

    SDL_RenderPresent(s_renderer);
}

static void handle_bezel_click(int x, int y) {
    uint32_t now = SDL_GetTicks();
    for (size_t i = 0; i < NUM_BUTTONS; i++) {
        bezel_btn_t *b = &s_buttons[i];
        if (x >= b->rect.x && x < b->rect.x + b->rect.w &&
            y >= b->rect.y && y < b->rect.y + b->rect.h) {
            b->flash_until = now + 150;
            if (b->key_code == 1) { // Power
                printf("[Sim Bezel] Clicked POWER button\n");
                sim_hw_inject_power();
            } else if (b->key_code == 2) { // Vol+
                printf("[Sim Bezel] Clicked VOL+ button\n");
                sim_hw_inject_volume(1);
            } else if (b->key_code == 3) { // Vol-
                printf("[Sim Bezel] Clicked VOL- button\n");
                sim_hw_inject_volume(-1);
            } else if (b->key_code == 4) { // AI
                printf("[Sim Bezel] Clicked AI button\n");
                sim_hw_inject_ai(METALIO_AI_SHORT);
            }
            sim_display_mark_dirty();
            break;
        }
    }
}

static void sim_poll_events(void) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            printf("[Sim] Window closed by user, exiting.\n");
            s_running = false;
            exit(0);
        } else if (e.type == SDL_MOUSEBUTTONDOWN) {
            if (e.button.button == SDL_BUTTON_LEFT) {
                if (e.button.x < SCREEN_W) {
                    s_mouse_down = true;
                    sim_touch_set(true, (uint16_t)e.button.x, (uint16_t)e.button.y);
                } else {
                    handle_bezel_click(e.button.x, e.button.y);
                }
            }
        } else if (e.type == SDL_MOUSEMOTION) {
            if (s_mouse_down && e.motion.x < SCREEN_W) {
                sim_touch_set(true, (uint16_t)e.motion.x, (uint16_t)e.motion.y);
            }
        } else if (e.type == SDL_MOUSEBUTTONUP) {
            if (e.button.button == SDL_BUTTON_LEFT) {
                s_mouse_down = false;
                sim_touch_set(false, (uint16_t)e.button.x, (uint16_t)e.button.y);
            }
        } else if (e.type == SDL_KEYDOWN) {
            SDL_Keycode k = e.key.keysym.sym;
            if (k == SDLK_UP || k == SDLK_k || k == SDLK_PAGEUP) {
                printf("[Sim Key] VOL+ (Prev)\n");
                sim_hw_inject_volume(1);
                s_buttons[1].flash_until = SDL_GetTicks() + 150;
                sim_display_mark_dirty();
            } else if (k == SDLK_DOWN || k == SDLK_j || k == SDLK_PAGEDOWN) {
                printf("[Sim Key] VOL- (Next)\n");
                sim_hw_inject_volume(-1);
                s_buttons[2].flash_until = SDL_GetTicks() + 150;
                sim_display_mark_dirty();
            } else if (k == SDLK_a || k == SDLK_m) {
                printf("[Sim Key] AI (Menu)\n");
                sim_hw_inject_ai(METALIO_AI_SHORT);
                s_buttons[3].flash_until = SDL_GetTicks() + 150;
                sim_display_mark_dirty();
            } else if (k == SDLK_p || k == SDLK_ESCAPE) {
                printf("[Sim Key] POWER (Lock/Wake)\n");
                sim_hw_inject_power();
                s_buttons[0].flash_until = SDL_GetTicks() + 150;
                sim_display_mark_dirty();
            } else if (k == SDLK_q) {
                printf("[Sim Key] Q pressed -> exit\n");
                exit(0);
            }
        }
    }
}

static pthread_t s_main_thread_id;

static bool s_test_mode = false;
static int s_test_frame = 0;

static void sim_save_bmp(const char *filename) {
    SDL_Surface *surf = SDL_CreateRGBSurfaceFrom(
        s_pixels, SCREEN_W, SCREEN_H, 32, SCREEN_W * sizeof(uint32_t),
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000
    );
    if (surf) {
        SDL_SaveBMP(surf, filename);
        SDL_FreeSurface(surf);
        printf("[Test] Saved screenshot to %s\n", filename);
    }
}

// Hook called by FreeRTOS on every loop tick!
void vTaskDelay(uint32_t ticks) {
    if (pthread_equal(pthread_self(), s_main_thread_id)) {
        sim_poll_events();

        if (s_test_mode) {
            s_test_frame++;
            if (s_test_frame == 10) {
                printf("[Test] Clicking Tab 1: Kệ sách (x=250, y=1150)\n");
                sim_touch_set(true, 250, 1150);
            } else if (s_test_frame == 13) {
                sim_touch_set(false, 250, 1150);
            } else if (s_test_frame == 25) {
                sim_save_bmp("screenshot_shelf.bmp");
            } else if (s_test_frame == 30) {
                printf("[Test] Opening book 'sample' (x=342, y=230)\n");
                sim_touch_set(true, 342, 230);
            } else if (s_test_frame == 33) {
                sim_touch_set(false, 342, 230);
            } else if (s_test_frame == 50) {
                sim_save_bmp("screenshot_reading.bmp");
            } else if (s_test_frame == 55) {
                printf("[Test] Next page via VOL-\n");
                sim_hw_inject_volume(-1);
                sim_display_mark_dirty();
            } else if (s_test_frame == 70) {
                sim_save_bmp("screenshot_page2.bmp");
            } else if (s_test_frame == 80) {
                printf("[Test] ALL TEST STEPS COMPLETED SUCCESSFULLY!\n");
                exit(0);
            }
        }

        if (sim_display_is_dirty()) {
            sim_render_screen();
            sim_display_clear_dirty();
        }
        SDL_Delay(ticks > 0 ? ticks : 1);
    } else {
        usleep((ticks > 0 ? ticks : 1) * 1000);
    }
}

int main(int argc, char **argv) {
    s_main_thread_id = pthread_self();
    if (argc > 1 && strcmp(argv[1], "--test") == 0) {
        s_test_mode = true;
        printf("[Sim] Running in automated TEST mode\n");
    }
    printf("========================================================\n");
    printf("  zOnhand dpReader31 E-Ink Simulator (Metalio 4 Plus)\n");
    printf("  Resolution: 684 x 1216 | 4-bit Grayscale E-Ink\n");
    printf("========================================================\n");

    // Initialize VFS storage mapping
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd))) {
        char storage_path[1024];
        snprintf(storage_path, sizeof(storage_path), "%s/storage", cwd);
        sim_vfs_init(storage_path);
        printf("[Sim] Storage initialized at: %s\n", storage_path);
    } else {
        sim_vfs_init("./storage");
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    s_window = SDL_CreateWindow(
        "zOnhand - dpReader31 (Metalio 4 Plus)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WIN_INIT_W, WIN_INIT_H,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );
    if (!s_window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return 1;
    }

    s_renderer = SDL_CreateRenderer(
        s_window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (!s_renderer) {
        s_renderer = SDL_CreateRenderer(s_window, -1, 0);
    }
    if (!s_renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_RenderSetLogicalSize(s_renderer, TOTAL_W, TOTAL_H);

    s_screen_texture = SDL_CreateTexture(
        s_renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_W, SCREEN_H
    );
    if (!s_screen_texture) {
        fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
        return 1;
    }

    // Allocate real 4bpp framebuffer: 1216 * 684 / 2 = 415,872 bytes
    size_t fb_size = (1216 * 684) / 2;
    uint8_t *framebuffer = (uint8_t*)malloc(fb_size);
    if (!framebuffer) {
        fprintf(stderr, "Failed to allocate framebuffer (%zu bytes)\n", fb_size);
        return 1;
    }
    memset(framebuffer, 0xFF, fb_size); // all white
    sim_display_set_framebuffer(framebuffer);

    // Initial render
    sim_render_screen();

    // Initialize firmware subsystem
    printf("[Sim] Initializing dpReader settings...\n");
    app_settings_init();
    pico_boot_init(false);

    printf("[Sim] Activating built-in TTF fonts...\n");
    ttf_font_open_builtin();
    app_font_activate_system();

    printf("[Sim] Launching dpReader app loop...\n");
    EpdiyHighlevelState hl = {0};

    const app_desc_t *first_app = app_home_page();
    printf("[Sim] First page: %s\n", first_app ? first_app->title : "none");

    app_loop_config_t cfg = {
        .hl = &hl,
        .fb = framebuffer,
        .acc = NULL,
        .tp = (void*)1, // non-null touch handle
        .sensor_ready = false,
        .first_app = first_app,
    };

    // Run firmware event loop!
    app_loop_run(&cfg);

    printf("[Sim] app_loop exited.\n");
    free(framebuffer);
    SDL_DestroyTexture(s_screen_texture);
    SDL_DestroyRenderer(s_renderer);
    SDL_DestroyWindow(s_window);
    SDL_Quit();
    return 0;
}
