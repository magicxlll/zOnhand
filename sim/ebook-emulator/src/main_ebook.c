#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lvgl/lvgl.h"

#include "src/lib/driver_backends.h"
#include "src/lib/simulator_util.h"
#include "src/lib/simulator_settings.h"

#include "book_reader_emulator.h"

extern simulator_settings_t settings;

static char* selected_backend;

static void configure_simulator(int argc, char** argv) {
    int opt = 0;
    selected_backend = NULL;
    driver_backends_register();

    // Display = e-paper panel + virtual-key strip (T5): 684+44 = 728 wide.
    settings.window_width = EPD_SIM_VIEW_W;
    settings.window_height = EPD_VER_RES;

    while ((opt = getopt(argc, argv, "b:W:H:h")) != -1) {
        switch (opt) {
            case 'h':
                fprintf(stdout, "ebook-emulator [-b sdl] [-W width] [-H height]\n");
                exit(EXIT_SUCCESS);
            case 'b':
                if (driver_backends_is_supported(optarg) == 0) {
                    die("error no such backend: %s\n", optarg);
                }
                selected_backend = strdup(optarg);
                break;
            case 'W':
                settings.window_width = atoi(optarg);
                break;
            case 'H':
                settings.window_height = atoi(optarg);
                break;
            default:
                break;
        }
    }
}

int main(int argc, char** argv) {
    configure_simulator(argc, argv);

    lv_init();

    if (driver_backends_init_backend(selected_backend) == -1) {
        die("Failed to initialize display backend");
    }

    const char* epdfont = getenv("EBOOK_EPDFONT");
    const char* ebook = getenv("EBOOK_PATH");

    if (epdfont != NULL) {
        book_reader_set_epdfont_path(epdfont);
    } else {
        // Default to the bundled .ef. NOTE: upstream `emulator/assets/fonts/*.ef`
        // are broken symlinks that materialize as 88-byte text files (the original
        // author's absolute paths), which make epdfont_open() return NULL and crash
        // BuildScreen(). This copy repairs them to point at tools/epdfont/MiSans-Light/.
        book_reader_set_epdfont_path("assets/fonts/misans_25_2.ef");
    }

    book_reader_init();
    book_reader_input_init();

    if (ebook) {
        if (!book_reader_open_ebook_path(ebook)) {
            fprintf(stderr, "open ebook failed: %s\n", book_reader_last_error());
        }
    }

    driver_backends_run_loop();
    return 0;
}
