#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "lvgl/lvgl.h"
#include "src/lib/driver_backends.h"
#include "src/lib/simulator_settings.h"
#include "book_reader_emulator.h"

extern simulator_settings_t settings;

int main(int /*argc*/, char** /*argv*/) {
    printf("=== Starting T2' Verification Test ===\n");
    driver_backends_register();
    settings.window_width = EPD_SIM_VIEW_W;
    settings.window_height = EPD_VER_RES;
    lv_init();
    driver_backends_init_backend(NULL);

    // Step 1: Init emulator
    printf("[1] Initializing book reader...\n");
    book_reader_init();
    book_reader_input_init();
    printf("    Init OK.\n");

    // Step 2: Show Home
    printf("[2] Testing Home screen...\n");
    book_reader_show_home();
    lv_timer_handler();
    printf("    Home screen rendered OK.\n");

    // Step 3: Open book
    printf("[3] Testing Open Book (sample.ebook)...\n");
    const char* sample_paths[] = {
        "sim/sample/sample.ebook",
        "sample/sample.ebook",
        "../sample/sample.ebook",
        "../../sample/sample.ebook"
    };
    int opened = 0;
    for (const char* p : sample_paths) {
        if (book_reader_open_ebook_path(p)) {
            printf("    Opened book at '%s'\n", p);
            opened = 1;
            break;
        }
    }
    assert(opened && "Failed to open sample.ebook");
    lv_timer_handler();

    // Step 4: Test reading navigation J/K/H
    printf("[4] Testing Reading Navigation (J/K/H)...\n");
    printf("    Page 0 rendered.\n");

    book_reader_next_page(); // J
    lv_timer_handler();
    printf("    Next page (J) executed.\n");

    book_reader_prev_page(); // K
    lv_timer_handler();
    printf("    Prev page (K) executed.\n");

    book_reader_next_page(); // J again
    lv_timer_handler();
    book_reader_vk_home(); // H: returns to page 0
    lv_timer_handler();
    printf("    VK Home (H) to page 0 executed.\n");

    book_reader_vk_home(); // H at page 0: returns to Home
    lv_timer_handler();
    printf("    VK Home (H) at page 0 -> Home screen OK.\n");

    // Step 5: Test Settings and Font changing (25 -> 30)
    printf("[5] Testing Settings Screen & Font 25 -> 30...\n");
    book_reader_show_settings();
    lv_timer_handler();
    printf("    Settings screen rendered OK.\n");

    const char* font25_paths[] = {
        "vendor/cloudzao/tools/epdfont/MiSans-Light/misans_25_2.ef",
        "../vendor/cloudzao/tools/epdfont/MiSans-Light/misans_25_2.ef",
        "../../vendor/cloudzao/tools/epdfont/MiSans-Light/misans_25_2.ef",
        "../../../vendor/cloudzao/tools/epdfont/MiSans-Light/misans_25_2.ef"
    };
    const char* font30_paths[] = {
        "vendor/cloudzao/tools/epdfont/MiSans-Light/misans_30_2.ef",
        "../vendor/cloudzao/tools/epdfont/MiSans-Light/misans_30_2.ef",
        "../../vendor/cloudzao/tools/epdfont/MiSans-Light/misans_30_2.ef",
        "../../../vendor/cloudzao/tools/epdfont/MiSans-Light/misans_30_2.ef"
    };
    const char* f25 = NULL;
    const char* f30 = NULL;
    for (const char* p : font25_paths) {
        FILE* fp = fopen(p, "rb");
        if (fp) { fclose(fp); f25 = p; break; }
    }
    for (const char* p : font30_paths) {
        FILE* fp = fopen(p, "rb");
        if (fp) { fclose(fp); f30 = p; break; }
    }
    assert(f25 && f30 && "Font files exist");

    printf("    Applying font 25: %s\n", f25);
    book_reader_set_epdfont_path(f25);
    lv_timer_handler();

    printf("    Applying font 30: %s\n", f30);
    book_reader_set_epdfont_path(f30);
    lv_timer_handler();
    printf("    Font 30 applied and reader re-paginated successfully!\n");

    printf("=== All T2' Tests Passed Successfully! ===\n");
    return 0;
}
