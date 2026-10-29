#ifndef _AUDIO_NOOP_H_
#define _AUDIO_NOOP_H_

#include <vector>
#include <cstdint>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif

void MetalioAudio_Init(void);
void MetalioAudio_RequestModeInit(void);
void MetalioAudio_OnPowerOff(void);
void MetalioAudio_OnPowerRestored(void);
bool MetalioAudio_WaitModeReady(int timeout_ms);
bool MetalioAudio_EnsureStarted(void);
bool MetalioAudio_ModeReady(void);
void MetalioAudio_SetAmpState(bool enable);
bool MetalioAudio_GetAmpState(void);

#ifdef __cplusplus
}
#endif

namespace sim {

class AudioNoop {
public:
    static AudioNoop& GetInstance() {
        static AudioNoop instance;
        return instance;
    }

    bool is_initialized() const { return initialized_; }
    void set_initialized(bool init) { initialized_ = init; }

private:
    AudioNoop() = default;
    bool initialized_ = false;
};

} // namespace sim

#endif // _AUDIO_NOOP_H_
