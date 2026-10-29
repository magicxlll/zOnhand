cmake_minimum_required(VERSION 3.10)
project(freertos_shim C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_library(freertos_shim STATIC
    src/freertos_shim.cpp
    src/esp_timer_shim.cpp
)

target_include_directories(freertos_shim PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(freertos_shim PUBLIC pthread)

enable_testing()
add_executable(test_freertos_shim test/test_freertos.cpp)
target_link_libraries(test_freertos_shim freertos_shim)
add_test(NAME test_freertos_shim COMMAND test_freertos_shim)
