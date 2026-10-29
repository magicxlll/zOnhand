#include "audio_noop.h"
#include <cstdio>

static bool s_amp_state = false;

// C++ linkage definitions matching metalio_audio.h
void MetalioAudio_Init() {
    sim::AudioNoop::GetInstance().set_initialized(true);
    std::fprintf(stderr, "[Audio Shim] MetalioAudio_Init() no-op\n");
}

void MetalioAudio_RequestModeInit() {
    std::fprintf(stderr, "[Audio Shim] MetalioAudio_RequestModeInit() no-op\n");
}

void MetalioAudio_OnPowerOff() {
}

void MetalioAudio_OnPowerRestored() {
}

bool MetalioAudio_WaitModeReady(int timeout_ms) {
    (void)timeout_ms;
    return false;
}

bool MetalioAudio_EnsureStarted() {
    return false;
}

bool MetalioAudio_ModeReady() {
    return false;
}

void MetalioAudio_SetAmpState(bool enable) {
    s_amp_state = enable;
}

bool MetalioAudio_GetAmpState() {
    return s_amp_state;
}

// C linkage aliases for C callers if any
extern "C" {

void c_MetalioAudio_Init(void) { MetalioAudio_Init(); }
bool c_MetalioAudio_ModeReady(void) { return MetalioAudio_ModeReady(); }

} // extern "C"
