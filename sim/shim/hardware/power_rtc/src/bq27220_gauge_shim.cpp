#include "bq27220_gauge.h"
#include "sim_power_rtc.h"

#include <cstdlib>
#include <fstream>
#include <mutex>
#include <string>

namespace {

std::mutex s_battery_mtx;
int s_battery_level = 70; // 70% default (marketing baseline)
uint16_t s_battery_voltage_mv = 3850;
bool s_charging = false;
bool s_discharging = true;

void RefreshFromEnvOrFileLocked() {
    // Check environment variable ZONHAND_BATTERY_LEVEL
    const char* env_level = std::getenv("ZONHAND_BATTERY_LEVEL");
    if (env_level != nullptr) {
        int lvl = std::atoi(env_level);
        if (lvl >= 0 && lvl <= 100) {
            s_battery_level = lvl;
        }
    }
    // Check file ./fs_/.sim_battery
    std::ifstream file("./fs_/.sim_battery");
    if (file.is_open()) {
        int lvl = 0;
        if (file >> lvl && lvl >= 0 && lvl <= 100) {
            s_battery_level = lvl;
        }
    }
}

} // namespace

namespace sim::hw {

void SetBattery(int level, uint16_t voltage_mv, bool charging, bool discharging) {
    std::lock_guard<std::mutex> lock(s_battery_mtx);
    if (level < 0) level = 0;
    if (level > 100) level = 100;
    s_battery_level = level;
    s_battery_voltage_mv = voltage_mv;
    s_charging = charging;
    s_discharging = discharging;
}

void GetBattery(int& level, uint16_t& voltage_mv, bool& charging, bool& discharging) {
    std::lock_guard<std::mutex> lock(s_battery_mtx);
    RefreshFromEnvOrFileLocked();
    level = s_battery_level;
    voltage_mv = s_battery_voltage_mv;
    charging = s_charging;
    discharging = s_discharging;
}

void ResetBatteryDefault() {
    std::lock_guard<std::mutex> lock(s_battery_mtx);
    s_battery_level = 70;
    s_battery_voltage_mv = 3850;
    s_charging = false;
    s_discharging = true;
}

} // namespace sim::hw

// ===========================================================================
// Bq27220Gauge implementation
// ===========================================================================

bool Bq27220Gauge::Begin(const power_i2c_t* /*bus*/, uint8_t addr) {
    addr_ = addr;
    ready_ = true;
    displayed_soc_ = 70;
    filter_sum_ = 70.0f;
    return true;
}

bool Bq27220Gauge::ReadVoltageMv(uint16_t& mv) {
    if (!ready_) return false;
    int lvl = 0;
    bool chg = false, dis = false;
    sim::hw::GetBattery(lvl, mv, chg, dis);
    return true;
}

bool Bq27220Gauge::ReadCurrentMa(int16_t& current_ma) {
    if (!ready_) return false;
    int lvl = 0;
    uint16_t mv = 0;
    bool chg = false, dis = false;
    sim::hw::GetBattery(lvl, mv, chg, dis);
    if (chg) {
        current_ma = 500;
    } else if (dis) {
        current_ma = -150;
    } else {
        current_ma = 0;
    }
    return true;
}

bool Bq27220Gauge::GetBatteryLevel(int& level, bool& charging, bool& discharging) {
    if (!ready_) return false;
    uint16_t mv = 0;
    sim::hw::GetBattery(level, mv, charging, discharging);
    return true;
}

void Bq27220Gauge::ResetFilter() {
    filter_sum_ = 0.0f;
    filter_count_ = 0;
    filter_primed_ = false;
    displayed_soc_ = -1;
    full_above_since_us_ = 0;
}

bool Bq27220Gauge::ReadU16(uint8_t /*reg*/, uint16_t* out) {
    if (out != nullptr) *out = 0;
    return true;
}

float Bq27220Gauge::FilterPush(float sample) {
    filter_sum_ = sample;
    return sample;
}

int Bq27220Gauge::ApplyMonoStep(int target_pct, bool /*charging*/) {
    displayed_soc_ = target_pct;
    return target_pct;
}

extern "C" {

signed int battery_get_bat_voltage(void) {
    uint16_t mv = 3850;
    Bq27220Gauge::GetInstance().GetVoltageMv(mv);
    return static_cast<signed int>(mv);
}

signed int battery_get_bat_current(void) {
    int16_t ma = -150;
    Bq27220Gauge::GetInstance().ReadCurrentMa(ma);
    // 0.1uA scale -> 1mA = 10000
    return static_cast<signed int>(ma) * 10000;
}

} // extern "C"
