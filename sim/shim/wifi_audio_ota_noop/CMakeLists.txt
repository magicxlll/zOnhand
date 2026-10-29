cmake_minimum_required(VERSION 3.10)
project(wifi_audio_ota_noop_shim C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT TARGET freertos_shim)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../freertos freertos_build)
endif()
if(NOT TARGET nvs_shim)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../nvs nvs_build)
endif()

add_library(wifi_audio_ota_noop_shim STATIC
    src/wifi_noop.cpp
    src/audio_noop.cpp
    src/ota_noop.cpp
    src/esp_misc_shim.cpp
)

target_include_directories(wifi_audio_ota_noop_shim PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../freertos/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../nvs/include
)

target_link_libraries(wifi_audio_ota_noop_shim PUBLIC
    freertos_shim
    nvs_shim
)

enable_testing()
add_executable(test_wifi_audio_ota_shim test/test_wifi_audio_ota.cpp)
target_link_libraries(test_wifi_audio_ota_shim
    wifi_audio_ota_noop_shim
    nvs_shim
    freertos_shim
)
add_test(NAME test_wifi_audio_ota_shim COMMAND test_wifi_audio_ota_shim)
