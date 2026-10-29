#include "board.h"
#include "display.h"
#include "wifi_noop.h"
#include "audio_noop.h"
#include "ota_noop.h"

#include "standby_screen/standby_screen.h"
#include "assistant_screen/assistant_screen.h"
#include "task_screen/task_screen.h"
#include "ota_upgrade_screen/ota_upgrade_screen.h"
#include "ota_confirm_dialog/ota_confirm_dialog.h"
#include "system_info.h"
#include "power_policy.h"
#include "device_state_event.h"
#include "SdCardManager.hpp"
#include "mcp_server.h"
#include "audio_service.h"
#include "websocket_protocol.h"
#include "mqtt_protocol.h"
#include "api_endpoints.h"
#include "fontpack_lvgl.h"
#include "reader/book_home_snapshot.h"
#include "reader/book_progress_sync.h"
#include "reader/book_library_warmup.h"
#include "device_wifi_location.h"
#include "boot_key_handler.h"
#include "a2ui_img_cache.h"
#include "assistant_screen/assistant_chat_store.h"
#include "ota.h"
#include "processors/audio_debugger.h"
#include "opus_resampler.h"
#include <opus_encoder.h>
#include <opus_decoder.h>
#include "assets.h"
#include "web_socket.h"

#include <cstdio>
#include <cstring>
#include <memory>

class SimDisplay : public Display {
public:
    SimDisplay() {
        width_ = 684;
        height_ = 1216;
    }
    bool Lock(int = 0) override { return true; }
    void Unlock() override {}
    void SetChatMessage(const char* role, const char* content) override {
        std::printf("[SimDisplay] SetChatMessage: [%s] %s\n", role ? role : "", content ? content : "");
    }
    void UpdateStatusBar(bool update_all = false) override {
        std::printf("[SimDisplay] UpdateStatusBar(update_all=%d)\n", update_all);
    }
};

class SimBoard : public Board {
public:
    SimBoard() {
        display_ = new SimDisplay();
    }
    ~SimBoard() override {
        delete display_;
    }
    std::string GetBoardType() override { return "metalio-e-ink4-plus-sim"; }
    Display* GetDisplay() override { return display_; }
    AudioCodec* GetAudioCodec() override { return nullptr; }
    NetworkInterface* GetNetwork() override { return nullptr; }
    void StartNetwork() override {
        sim::WifiNoop::GetInstance().StartNetwork();
    }
    bool EnsureNetworkReady(int = 30000) override { return false; }
    bool IsNetworkReady() override { return sim::WifiNoop::GetInstance().IsNetworkReady(); }
    bool IsWifiConfigMode() override { return sim::WifiNoop::GetInstance().IsWifiConfigMode(); }
    const char* GetNetworkStateIcon() override { return ""; }
    const char* GetNetworkBusyStatusText() override { return "offline"; }
    void SetPowerSaveMode(bool) override {}
    std::string GetBoardJson() override { return "{}"; }
    std::string GetDeviceStatusJson() override { return "{}"; }
private:
    SimDisplay* display_ = nullptr;
};

void* create_board() {
    static SimBoard board;
    return &board;
}

// StandbyScreen stubs
bool StandbyScreen::IsActive() { return false; }
void StandbyScreen::EnsureWeatherCached() {
    std::printf("[StandbyScreen] EnsureWeatherCached() called\n");
}

// AssistantScreen stubs
bool AssistantScreen::IsActive() { return false; }
bool AssistantScreen::IsPttHeld() { return false; }
void AssistantScreen::SyncPttOverlay() {}
void AssistantScreen::AddYunxinSpeech(const char*) {}
void AssistantScreen::FlushYunxinSpeech() {}
void AssistantScreen::FinishYunxinSpeech() {}

// TaskScreen stubs
void TaskScreen::EnsureCacheSynced() {
    std::printf("[TaskScreen] EnsureCacheSynced() called\n");
}

// OtaScreens stubs
OtaConfirmDialog::Choice OtaConfirmDialog::ShowBlocking(const char*, const char*, Mode) {
    return OtaConfirmDialog::Choice::Failed;
}
void OtaUpgradeScreen::SetProgress(int) {}
bool OtaUpgradeScreen::Show(const char*, const char*) { return false; }
void OtaUpgradeScreen::Dismiss() {}
void OtaUpgradeScreen::SetHint(const char*) {}

// Hardware & Boot Key stubs
bool BootKey_IsHeld() { return false; }
void SdLogWriter_Start() { std::printf("[SdLogWriter] Started (sim file-based)\n"); }
bool fontpack_lv_ensure_ready() { return true; }
const lv_font_t* fontpack_lv_font_ui(void) { return nullptr; }
esp_err_t a2ui_img_cache_init() { return ESP_OK; }
esp_err_t assistant_chat_store_init() { return ESP_OK; }

// Reader & Location warmup stubs
namespace reader {
namespace book_home_snapshot {
    void HydrateFromNvs() {
        std::printf("[Reader] book_home_snapshot::HydrateFromNvs()\n");
    }
}
namespace book_library_warmup {
    void RequestBootWarmup() {
        std::printf("[Reader] book_library_warmup::RequestBootWarmup()\n");
    }
}
namespace book_progress_sync {
    void RequestBootReport() {}
}
}

namespace device_wifi_location {
    void RequestBootReport() {}
}

// SystemInfo stubs
std::string SystemInfo::GetUserAgent() {
    return "Metalio-E-Ink4-Plus/1.0.0-sim (zOnhand Simulator)";
}
std::string SystemInfo::GetMacAddress() {
    return "02:00:00:00:00:01";
}
size_t SystemInfo::GetFlashSize() { return 16 * 1024 * 1024; }
size_t SystemInfo::GetMinimumFreeHeapSize() { return 8 * 1024 * 1024; }
size_t SystemInfo::GetFreeHeapSize() { return 12 * 1024 * 1024; }
std::string SystemInfo::GetChipModelName() { return "esp32s31"; }

// Audio helpers
AudioDebugger::~AudioDebugger() = default;
OpusResampler::OpusResampler() : input_sample_rate_(16000), output_sample_rate_(16000) {}
OpusResampler::~OpusResampler() = default;
OpusEncoderWrapper::~OpusEncoderWrapper() = default;
OpusDecoderWrapper::~OpusDecoderWrapper() = default;

// WebSocket stub
WebSocket::~WebSocket() = default;

// Assets stub
Assets::Assets() = default;
Assets::~Assets() = default;
bool Assets::Apply() { return true; }
bool Assets::Download(std::string, std::function<void(int, size_t)>) { return false; }
bool Assets::GetAssetData(const std::string&, void*&, size_t&) { return false; }

// DeviceStateEventManager stubs
DeviceStateEventManager::DeviceStateEventManager() = default;
DeviceStateEventManager::~DeviceStateEventManager() = default;

// PowerPolicy stubs
PowerPolicy& PowerPolicy::GetInstance() {
    static PowerPolicy instance;
    return instance;
}
void PowerPolicy::NotifyDeviceState(DeviceState state) {
    (void)state;
}
void PowerPolicy::Acquire(PowerNeed need) {
    (void)need;
}
void PowerPolicy::Release(PowerNeed need) {
    (void)need;
}

// DeviceStateEventManager stubs
DeviceStateEventManager& DeviceStateEventManager::GetInstance() {
    static DeviceStateEventManager instance;
    return instance;
}
void DeviceStateEventManager::PostStateChangeEvent(DeviceState old_state, DeviceState new_state) {
    std::printf("[DeviceState] Event: %d -> %d\n", static_cast<int>(old_state), static_cast<int>(new_state));
}

// SdCardManager stubs
SdCardManager& SdCardManager::GetInstance() {
    static SdCardManager instance;
    return instance;
}
bool SdCardManager::IsMounted() const { return false; }

// McpServer stubs
McpServer::McpServer() = default;
McpServer::~McpServer() = default;
void McpServer::ParseMessage(const cJSON*) {}
void McpServer::AddCommonTools() {
    std::printf("[McpServer] AddCommonTools()\n");
}
void McpServer::AddUserOnlyTools() {
    std::printf("[McpServer] AddUserOnlyTools()\n");
}

// AudioService stubs
AudioService::AudioService() {}
AudioService::~AudioService() {}
const std::string& AudioService::GetLastWakeWord() const { static std::string s = ""; return s; }
void AudioService::Initialize(AudioCodec*) {}
void AudioService::ResetDecoder() {}
void AudioService::SetCallbacks(AudioServiceCallbacks&) {}
void AudioService::SetModelsList(srmodel_list*) {}
void AudioService::EncodeWakeWord() {}
void AudioService::EnableDeviceAec(bool) {}
void AudioService::EnableAudioTesting(bool) {}
void AudioService::EnableVoiceProcessing(bool) {}
std::unique_ptr<AudioStreamPacket> AudioService::PopPacketFromSendQueue() { return nullptr; }
void AudioService::EnableWakeWordDetection(bool) {}
bool AudioService::PushPacketToDecodeQueue(std::unique_ptr<AudioStreamPacket>, bool) { return true; }
void AudioService::Stop() {}
void AudioService::Start() {}
bool AudioService::IsIdle() { return true; }

// Protocols stubs
MqttProtocol::MqttProtocol() {}
MqttProtocol::~MqttProtocol() {}
bool MqttProtocol::Start() { return true; }
bool MqttProtocol::SendAudio(std::unique_ptr<AudioStreamPacket>) { return true; }
bool MqttProtocol::OpenAudioChannel() { return true; }
void MqttProtocol::CloseAudioChannel() {}
bool MqttProtocol::IsAudioChannelOpened() const { return false; }

bool MqttProtocol::SendText(const std::string&) { return true; }

WebsocketProtocol::WebsocketProtocol() {}
WebsocketProtocol::~WebsocketProtocol() {}
bool WebsocketProtocol::Start() { return true; }
bool WebsocketProtocol::SendAudio(std::unique_ptr<AudioStreamPacket>) { return true; }
bool WebsocketProtocol::OpenAudioChannel() { return true; }
void WebsocketProtocol::CloseAudioChannel() {}
bool WebsocketProtocol::IsAudioChannelOpened() const { return false; }
bool WebsocketProtocol::SendText(const std::string&) { return true; }

// Ota implementation
Ota::Ota() : current_version_("1.0.0-sim") {}
Ota::~Ota() = default;
esp_err_t Ota::CheckVersion() { return ESP_OK; }
esp_err_t Ota::CheckChatConfiguration() { return ESP_OK; }
esp_err_t Ota::Activate() { return ESP_OK; }
void Ota::MarkCurrentVersionValid() {
    sim::OtaNoop::GetInstance().set_version_marked_valid(true);
    std::printf("[OTA] MarkCurrentVersionValid()\n");
}
bool Ota::StartUpgrade(std::function<void(int, size_t)>) { return false; }
bool Ota::StartUpgradeFromUrl(const std::string&, std::function<void(int, size_t)>) { return false; }
bool Ota::StartUpgradeFromFile(const std::string&, std::function<void(int, size_t)>) { return false; }
bool Ota::PeekFirmwareVersionFromFile(const std::string&, std::string&) { return false; }
std::string Ota::GetCheckVersionUrl() { return ""; }
bool Ota::Upgrade(const std::string&) { return false; }
bool Ota::UpgradeFromFile(const std::string&) { return false; }
std::vector<int> Ota::ParseVersion(const std::string&) { return {}; }
bool Ota::IsNewVersionAvailable(const std::string&, const std::string&) { return false; }
std::string Ota::GetActivationPayload() { return ""; }
std::unique_ptr<Http> Ota::SetupHttp() { return nullptr; }
esp_err_t Ota::CheckResponse(bool) { return ESP_OK; }
esp_err_t Ota::CheckResponseOnce(bool, int64_t) { return ESP_OK; }
