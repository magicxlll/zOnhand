cmake_minimum_required(VERSION 3.10)
project(tca9555_keys_shim C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../..")

if(NOT TARGET freertos_shim)
    add_subdirectory(${REPO_ROOT}/sim/shim/freertos freertos_build)
endif()

add_library(tca9555_keys_shim STATIC
    src/tca9555_shim.cpp
    ${REPO_ROOT}/vendor/cloudzao/main/boards/metalio-e-ink4-plus/input/metalio_keys.cc
)

target_include_directories(tca9555_keys_shim PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${REPO_ROOT}/vendor/cloudzao/main/boards/metalio-e-ink4-plus
    ${REPO_ROOT}/vendor/cloudzao/main/boards/metalio-e-ink4-plus/input
    ${REPO_ROOT}/vendor/cloudzao/main/boards/metalio-e-ink4-plus/display
    ${REPO_ROOT}/sim/shim/freertos/include
)

target_link_libraries(tca9555_keys_shim PUBLIC
    freertos_shim
    pthread
)

enable_testing()
add_executable(test_tca9555_keys test/test_tca9555_keys.cpp)
target_link_libraries(test_tca9555_keys tca9555_keys_shim)
add_test(NAME test_tca9555_keys COMMAND test_tca9555_keys)
