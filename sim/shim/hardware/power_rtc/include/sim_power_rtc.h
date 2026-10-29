#pragma once

#include <cstdint>
#include <ctime>

namespace sim::hw {

/**
 * @brief Set the simulated battery fuel gauge state.
 * @param level Battery level percentage (0..100)
 * @param voltage_mv Battery voltage in millivolts (e.g. 3850)
 * @param charging Whether battery is charging (current > +5mA)
 * @param discharging Whether battery is discharging (current < -5mA)
 */
void SetBattery(int level, uint16_t voltage_mv, bool charging, bool discharging);

/**
 * @brief Get the current simulated battery state.
 */
void GetBattery(int& level, uint16_t& voltage_mv, bool& charging, bool& discharging);

/**
 * @brief Reset simulated battery state to default (70%, 3850mV, discharging).
 */
void ResetBatteryDefault();

/**
 * @brief Set simulated RTC time.
 */
void SetRtcTime(const struct tm& time);

/**
 * @brief Get simulated RTC time.
 */
void GetRtcTime(struct tm& time);

} // namespace sim::hw
