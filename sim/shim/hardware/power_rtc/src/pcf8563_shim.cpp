#include "pcf8563.h"
#include "sim_power_rtc.h"

#include <chrono>
#include <ctime>
#include <mutex>

namespace {

std::mutex s_rtc_mtx;
bool s_custom_time_set = false;
struct tm s_custom_time = {};
std::chrono::steady_clock::time_point s_set_point;

} // namespace

namespace sim::hw {

void SetRtcTime(const struct tm& time) {
    std::lock_guard<std::mutex> lock(s_rtc_mtx);
    s_custom_time = time;
    s_custom_time_set = true;
    s_set_point = std::chrono::steady_clock::now();
}

void GetRtcTime(struct tm& time) {
    std::lock_guard<std::mutex> lock(s_rtc_mtx);
    if (s_custom_time_set) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed_sec = std::chrono::duration_cast<std::chrono::seconds>(now - s_set_point).count();
        time_t t = mktime(&s_custom_time) + elapsed_sec;
        localtime_r(&t, &time);
    } else {
        time_t now = ::time(nullptr);
        localtime_r(&now, &time);
    }
}

} // namespace sim::hw

// ===========================================================================
// Pcf8563 implementation
// ===========================================================================

bool Pcf8563::Begin(const power_i2c_t* /*bus*/, uint8_t /*addr*/) {
    ready_ = true;
    bus_valid_ = true;
    return true;
}

bool Pcf8563::GetTime(struct tm& out, bool* valid) {
    if (!ready_) return false;
    sim::hw::GetRtcTime(out);
    if (valid != nullptr) {
        *valid = true;
    }
    return true;
}

bool Pcf8563::SetTime(const struct tm& in) {
    if (!ready_) return false;
    sim::hw::SetRtcTime(in);
    return true;
}

bool Pcf8563::ApplyRtcToSystem() {
    if (!ready_) return false;
    struct tm tm_out = {};
    return GetTime(tm_out, nullptr);
}

bool Pcf8563::SyncSystemToRtc() {
    if (!ready_) return false;
    time_t now = ::time(nullptr);
    struct tm local = {};
    if (localtime_r(&now, &local) == nullptr) return false;
    return SetTime(local);
}

bool Pcf8563::ClearIrqFlags() {
    return true;
}

bool Pcf8563::WriteRegs(uint8_t /*reg*/, const uint8_t* /*data*/, size_t /*len*/) {
    return true;
}

bool Pcf8563::ReadRegs(uint8_t /*reg*/, uint8_t* /*data*/, size_t /*len*/) {
    return true;
}

uint8_t Pcf8563::DecToBcd(uint8_t v) {
    return ((v / 10) << 4) | (v % 10);
}

uint8_t Pcf8563::BcdToDec(uint8_t v) {
    return ((v >> 4) * 10) + (v & 0x0F);
}
