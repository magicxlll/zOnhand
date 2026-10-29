cmake_minimum_required(VERSION 3.10)
project(power_rtc_shim C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../..")

if(NOT TARGET freertos_shim)
    add_subdirectory(${REPO_ROOT}/sim/shim/freertos freertos_build)
endif()

if(NOT TARGET tca9555_keys_shim)
    add_subdirectory(${REPO_ROOT}/sim/shim/hardware/tca9555_keys tca9555_build)
endif()

add_library(power_rtc_shim STATIC
    src/bq27220_gauge_shim.cpp
    src/pcf8563_shim.cpp
    src/metalio_i2c1_stubs.cpp
    ${REPO_ROOT}/vendor/cloudzao/main/boards/metalio-e-ink4-plus/periph/metalio_rtc.cc
    ${REPO_ROOT}/vendor/cloudzao/main/boards/metalio-e-ink4-plus/periph/metalio_sys_mon.cc
)

target_include_directories(power_rtc_shim PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${REPO_ROOT}/vendor/cloudzao/main/boards/common/power
    ${REPO_ROOT}/vendor/cloudzao/main/boards/common/sensor
    ${REPO_ROOT}/vendor/cloudzao/main/boards/metalio-e-ink4-plus
    ${REPO_ROOT}/vendor/cloudzao/main/boards/metalio-e-ink4-plus/bus
    ${REPO_ROOT}/vendor/cloudzao/main/boards/metalio-e-ink4-plus/display
    ${REPO_ROOT}/vendor/cloudzao/main/boards/metalio-e-ink4-plus/periph
    ${REPO_ROOT}/vendor/cloudzao/main/display
    ${REPO_ROOT}/vendor/cloudzao/main
    ${REPO_ROOT}/sim/shim/hardware/tca9555_keys/include
    ${REPO_ROOT}/sim/shim/freertos/include
)

target_link_libraries(power_rtc_shim PUBLIC
    tca9555_keys_shim
    freertos_shim
    pthread
)

enable_testing()
add_executable(test_power_rtc test/test_power_rtc.cpp)
target_link_libraries(test_power_rtc power_rtc_shim)
add_test(NAME test_power_rtc COMMAND test_power_rtc)
