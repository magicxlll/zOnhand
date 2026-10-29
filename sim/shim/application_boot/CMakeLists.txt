cmake_minimum_required(VERSION 3.10)
project(application_boot C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)

set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
set(VENDOR_MAIN "${REPO_ROOT}/vendor/cloudzao/main")
set(VENDOR_MANAGED "${REPO_ROOT}/vendor/cloudzao/managed_components")

if(NOT TARGET freertos_shim)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../freertos freertos_build)
endif()
if(NOT TARGET nvs_shim)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../nvs nvs_build)
endif()
if(NOT TARGET wifi_audio_ota_noop_shim)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../wifi_audio_ota_noop wifi_build)
endif()

set(APP_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/../freertos/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../nvs/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../wifi_audio_ota_noop/include
    ${REPO_ROOT}/sim/ebook-emulator/src/esp_compat
    ${VENDOR_MANAGED}/lvgl__lvgl
    ${VENDOR_MANAGED}/espressif__cjson/cJSON
    ${VENDOR_MANAGED}/78__esp-opus-encoder/include
    ${VENDOR_MANAGED}/78__esp-opus/include
    ${VENDOR_MAIN}
    ${VENDOR_MAIN}/assets
    ${VENDOR_MAIN}/audio
    ${VENDOR_MAIN}/protocols
    ${VENDOR_MAIN}/reader
    ${VENDOR_MAIN}/display
    ${VENDOR_MAIN}/display/a2ui
    ${VENDOR_MAIN}/display/font
    ${VENDOR_MAIN}/display/lvgl_display
    ${VENDOR_MAIN}/display/screen
    ${VENDOR_MAIN}/display/screen/assistant_screen
    ${VENDOR_MAIN}/display/screen/standby_screen
    ${VENDOR_MAIN}/display/screen/task_screen
    ${VENDOR_MAIN}/display/screen/ota_upgrade_screen
    ${VENDOR_MAIN}/display/screen/ota_confirm_dialog
    ${VENDOR_MAIN}/boards/common/board
    ${VENDOR_MAIN}/boards/common/power
    ${VENDOR_MAIN}/boards/common/power_policy
    ${VENDOR_MAIN}/boards/common/sensor
    ${VENDOR_MAIN}/boards/common/storage
    ${VENDOR_MAIN}/boards/metalio-e-ink4-plus
    ${VENDOR_MAIN}/boards/metalio-e-ink4-plus/periph
    ${REPO_ROOT}/vendor/cloudzao/components/esp-ml307/include
    ${REPO_ROOT}/vendor/cloudzao/components/esp-wifi-connect/include
)

add_executable(test_boot_application
    test_boot_application.cpp
    app_stubs.cpp
    ${VENDOR_MAIN}/application.cc
    ${VENDOR_MAIN}/boards/common/board/board.cc
    ${VENDOR_MAIN}/settings.cc
    ${VENDOR_MAIN}/assets/lang_home.cc
    ${VENDOR_MAIN}/display/display.cc
    ${VENDOR_MAIN}/cloudzao_endpoints.c
    ${VENDOR_MAIN}/protocols/protocol.cc
    ${VENDOR_MAIN}/ota_ignore.cc
    ${VENDOR_MANAGED}/espressif__cjson/cJSON/cJSON.c
)

target_include_directories(test_boot_application PRIVATE ${APP_INCLUDES})
target_compile_definitions(test_boot_application PRIVATE
    BOARD_NAME="metalio-e-ink4-plus"
)

target_link_libraries(test_boot_application PRIVATE
    wifi_audio_ota_noop_shim
    nvs_shim
    freertos_shim
    pthread
)

enable_testing()
add_test(NAME test_boot_application COMMAND test_boot_application)
