#include "audio_noop.h"
#include <cstdio>

static bool s_amp_state = false;

extern "C" {

void MetalioAudio_Init(void) {
    sim::AudioNoop::GetInstance().set_initialized(true);
    std::fprintf(stderr, "[Audio Shim] MetalioAudio_Init() no-op\n");
}

void MetalioAudio_RequestModeInit(void) {
    std::fprintf(stderr, "[Audio Shim] MetalioAudio_RequestModeInit() no-op\n");
}

void MetalioAudio_OnPowerOff(void) {
}

void MetalioAudio_OnPowerRestored(void) {
}

bool MetalioAudio_WaitModeReady(int timeout_ms) {
    (void)timeout_ms;
    return false;
}

bool MetalioAudio_EnsureStarted(void) {
    return false;
}

bool MetalioAudio_ModeReady(void) {
    return false;
}

void MetalioAudio_SetAmpState(bool enable) {
    s_amp_state = enable;
}

bool MetalioAudio_GetAmpState(void) {
    return s_amp_state;
}

} // extern "C"
