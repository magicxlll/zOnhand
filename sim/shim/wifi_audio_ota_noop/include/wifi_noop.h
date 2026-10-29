#ifndef _WIFI_NOOP_H_
#define _WIFI_NOOP_H_

#include <string>
#include <memory>
#include "esp_err.h"

namespace sim {

class WifiNoop {
public:
    static WifiNoop& GetInstance() {
        static WifiNoop instance;
        return instance;
    }

    void StartNetwork();
    bool IsNetworkReady() const { return connected_; }
    bool IsWifiConfigMode() const { return config_mode_; }
    void SetConnected(bool connected) { connected_ = connected; }
    void SetConfigMode(bool config_mode) { config_mode_ = config_mode; }

private:
    WifiNoop() = default;
    bool connected_ = false;
    bool config_mode_ = false;
};

} // namespace sim

#endif // _WIFI_NOOP_H_
