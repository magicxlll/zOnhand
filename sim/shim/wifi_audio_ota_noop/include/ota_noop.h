#ifndef _OTA_NOOP_H_
#define _OTA_NOOP_H_

#include <string>
#include <functional>
#include "esp_err.h"

namespace sim {

class OtaNoop {
public:
    static OtaNoop& GetInstance() {
        static OtaNoop instance;
        return instance;
    }

    bool version_marked_valid() const { return version_marked_valid_; }
    void set_version_marked_valid(bool valid) { version_marked_valid_ = valid; }

private:
    OtaNoop() = default;
    bool version_marked_valid_ = false;
};

} // namespace sim

#endif // _OTA_NOOP_H_
