CC ?= clang
CXX ?= clang++
CFLAGS := -std=gnu11 -O2 -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -Wno-format -Wno-unused-variable \
          -DKIIKO_BOARD_METALIO47=1 \
          -Iinclude \
          -I../../vendor/dpReader31/main \
          -I../../vendor/dpReader31/main/app \
          -I../../vendor/dpReader31/main/apps \
          -I../../vendor/dpReader31/main/font \
          -I../../vendor/dpReader31/main/ui \
          -I../../vendor/dpReader31/main/book \
          -I../../vendor/dpReader31/main/factory \
          -I../../vendor/dpReader31/boards/metalio47/src/main \
          -I../../vendor/dpReader31/components/jpegdec/src \
          -I/opt/homebrew/include \
          -I/opt/homebrew/include/SDL2 \
          -include sim_vfs_hook.h

CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter \
            -DKIIKO_BOARD_METALIO47=1 \
            -Iinclude \
            -I../../vendor/dpReader31/main \
            -I../../vendor/dpReader31/boards/metalio47/src/main \
            -I../../vendor/dpReader31/components/jpegdec/src \
            -I/opt/homebrew/include

LDFLAGS := -L/opt/homebrew/lib -lSDL2 -lpng -lz -lm -lpthread

SIM_C_SRCS := src/sim_main.c \
              src/sim_display.c \
              src/sim_touch.c \
              src/sim_hw.c \
              src/sim_sd.c \
              src/sim_assets.c \
              src/sim_stubs.c \
              src/tjpgd.c \
              src/cJSON.c

SIM_CXX_SRCS := src/nvs_shim.cpp

VENDOR_CXX_SRCS := ../../vendor/dpReader31/components/jpegdec/src/JPEGDEC.cpp \
                   ../../vendor/dpReader31/components/jpegdec/src/jpegdec_shim.cpp

VENDOR_ROOT := ../../vendor/dpReader31/main

VENDOR_SRCS := $(VENDOR_ROOT)/app/app_registry.c \
               $(VENDOR_ROOT)/app/app_loop.c \
               $(VENDOR_ROOT)/apps/app_dashboard.c \
               $(VENDOR_ROOT)/apps/app_book.c \
               $(VENDOR_ROOT)/apps/book_toc.c \
               $(VENDOR_ROOT)/apps/app_files.c \
               $(VENDOR_ROOT)/apps/file_tree.c \
               $(VENDOR_ROOT)/apps/app_device_settings.c \
               $(VENDOR_ROOT)/apps/app_image.c \
               $(VENDOR_ROOT)/apps/app_boot_recovery.c \
               $(VENDOR_ROOT)/ui/ui_kit.c \
               $(VENDOR_ROOT)/ui/ui_text_edit.c \
               $(VENDOR_ROOT)/ui/ui_text_input.c \
               $(VENDOR_ROOT)/ui/ui_ime.c \
               $(VENDOR_ROOT)/ui/ui_keyboard.c \
               $(VENDOR_ROOT)/ui/ui_gesture.c \
               $(VENDOR_ROOT)/ui/ui_pinpad.c \
               $(VENDOR_ROOT)/ui/ui_power_dialog.c \
               $(VENDOR_ROOT)/ui/ui_quick_menu.c \
               $(VENDOR_ROOT)/ui/ui_quick_menu_input.c \
               $(VENDOR_ROOT)/ui/ui_ai_menu.c \
               $(VENDOR_ROOT)/ui/ui_lock_feedback.c \
               $(VENDOR_ROOT)/ui/ui_click_feedback.c \
               $(VENDOR_ROOT)/ui/ui_menu.c \
               $(VENDOR_ROOT)/ui/ui_nav.c \
               $(VENDOR_ROOT)/ui/ui_home_widget.c \
               $(VENDOR_ROOT)/ui/ui_home_clock.c \
               $(VENDOR_ROOT)/ui/ui_image_dither.c \
               $(VENDOR_ROOT)/ui/ui_wallpaper.c \
               $(VENDOR_ROOT)/ui/ui_avatar.c \
               $(VENDOR_ROOT)/ui/ui_reader_background.c \
               $(VENDOR_ROOT)/font/ttf_font.c \
               $(VENDOR_ROOT)/font/ui_font.c \
               $(VENDOR_ROOT)/font/app_font_context.c \
               $(VENDOR_ROOT)/settings.c \
               $(VENDOR_ROOT)/book/book_store.c \
               $(VENDOR_ROOT)/book/book_progress.c \
               $(VENDOR_ROOT)/book/book_title.c \
               $(VENDOR_ROOT)/book/book_ticket.c \
               $(VENDOR_ROOT)/book/book_excerpt.c \
               $(VENDOR_ROOT)/book/book_source.c \
               $(VENDOR_ROOT)/book/book_index_cache.c \
               $(VENDOR_ROOT)/book/book_txt.c \
               $(VENDOR_ROOT)/book/book_epub.c \
               $(VENDOR_ROOT)/book/book_cover.c \
               $(VENDOR_ROOT)/book/book_cover_auto.c \
               $(VENDOR_ROOT)/book/book_cover_jobs.c \
               $(VENDOR_ROOT)/book/zip_reader.c \
               $(VENDOR_ROOT)/book/html_text.c \
               $(VENDOR_ROOT)/book/book_layout.c \
               $(VENDOR_ROOT)/book/gbk.c \
               $(VENDOR_ROOT)/book/book_image_header.c \
               $(VENDOR_ROOT)/book/reader_page_cache.c \
               $(VENDOR_ROOT)/book/book_lock_collage.c \
               $(VENDOR_ROOT)/book/metalio_cover_cache.c \
               $(VENDOR_ROOT)/book/book_history_backup.c \
               $(VENDOR_ROOT)/boot_state.c \
               $(VENDOR_ROOT)/lock_screen.c \
               $(VENDOR_ROOT)/lock_pin_flow.c \
               $(VENDOR_ROOT)/anti_fade.c \
               $(VENDOR_ROOT)/app/sd_recovery.c \
               $(VENDOR_ROOT)/ui/ui_online_fonts.c \
               $(VENDOR_ROOT)/ui/datamatrix.c \
               $(VENDOR_ROOT)/font/ui_hanzi.c \
               ../../vendor/dpReader31/components/read_pico_search/read_pico_search.c

BUILD_DIR := build
OBJS := $(addprefix $(BUILD_DIR)/, $(SIM_C_SRCS:.c=.o)) \
        $(addprefix $(BUILD_DIR)/, $(SIM_CXX_SRCS:.cpp=.o)) \
        $(addprefix $(BUILD_DIR)/vendor/, $(notdir $(VENDOR_SRCS:.c=.o))) \
        $(addprefix $(BUILD_DIR)/vendor/, $(notdir $(VENDOR_CXX_SRCS:.cpp=.o)))

TARGET := dpReader_sim

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) -o $@
	@echo "Build complete: $(TARGET)"

$(BUILD_DIR)/src/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/src/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Pattern rules for vendor files
define VENDOR_RULE
$$(BUILD_DIR)/vendor/$$(notdir $(1:.c=.o)): $(1)
	@mkdir -p $$(dir $$@)
	$$(CC) $$(CFLAGS) -c $$< -o $$@
endef

$(foreach src,$(VENDOR_SRCS),$(eval $(call VENDOR_RULE,$(src))))

define VENDOR_CXX_RULE
$$(BUILD_DIR)/vendor/$$(notdir $(1:.cpp=.o)): $(1)
	@mkdir -p $$(dir $$@)
	$$(CXX) $$(CXXFLAGS) -c $$< -o $$@
endef

$(foreach src,$(VENDOR_CXX_SRCS),$(eval $(call VENDOR_CXX_RULE,$(src))))

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all clean
