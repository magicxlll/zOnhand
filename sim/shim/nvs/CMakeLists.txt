cmake_minimum_required(VERSION 3.10)
project(nvs_shim C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT TARGET freertos_shim)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../freertos freertos_build)
endif()

add_library(nvs_shim STATIC
    src/nvs_shim.cpp
)

target_include_directories(nvs_shim PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../freertos/include
)

target_link_libraries(nvs_shim PUBLIC freertos_shim)

enable_testing()
add_executable(test_nvs_shim test/test_nvs.cpp)
target_link_libraries(test_nvs_shim nvs_shim freertos_shim)
add_test(NAME test_nvs_shim COMMAND test_nvs_shim)
