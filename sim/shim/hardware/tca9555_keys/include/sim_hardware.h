#pragma once

#include <cstdint>

namespace sim::hw {

enum class Key {
    kBoot = 0,    // BOOT button (GPIO61, touch/physical bottom-left)
    kVolUp = 1,   // VOL+ (TCA P10, top right)
    kVolDown = 2, // VOL- (TCA P11, mid right)
    kPower = 3,   // POWER (TCA P17, bottom right)
};

/**
 * @brief Set the immediate pressed/released state of a key.
 * @param key Key identifier
 * @param pressed true = down/pressed, false = up/released
 */
void SetKey(Key key, bool pressed);

/**
 * @brief Simulate a complete key press for a given duration in milliseconds.
 * @param key Key identifier
 * @param hold_ms Duration to hold the key down before releasing
 */
void PressKey(Key key, uint32_t hold_ms);

/**
 * @brief Check if a key is currently held down.
 */
bool IsKeyDown(Key key);

/**
 * @brief Check current state of vibration motor output (TCA P0.3).
 */
bool GetMotorState();

/**
 * @brief Check current state of audio PA enable output (TCA P0.4).
 */
bool GetPaState();

/**
 * @brief Get the count of shutdown pulse trains sent via TCA P13.
 */
int GetShutdownPulseCount();

/**
 * @brief Reset simulated hardware states to default idle values.
 */
void ResetHardwareState();

} // namespace sim::hw
