#include "nvs.h"
#include "nvs_flash.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <vector>

namespace {

enum class NvsType {
    kInt64,
    kString,
    kBlob
};

struct NvsEntry {
    NvsType type = NvsType::kInt64;
    int64_t num_val = 0;
    std::string str_val;
    std::vector<uint8_t> blob_val;
};

struct NvsHandleContext {
    std::string ns_name;
    nvs_open_mode_t mode = NVS_READONLY;
};

static std::mutex s_nvs_mutex;
static std::map<std::string, std::map<std::string, NvsEntry>> s_namespaces;
static std::map<nvs_handle_t, NvsHandleContext> s_handles;
static nvs_handle_t s_next_handle = 1;
static bool s_initialized = false;

static std::string HexEncode(const uint8_t* data, size_t len) {
    std::ostringstream oss;
    for (size_t i = 0; i < len; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
    }
    return oss.str();
}

static std::vector<uint8_t> HexDecode(const std::string& hex) {
    std::vector<uint8_t> out;
    out.reserve(hex.size() / 2);
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        unsigned int byte = 0;
        std::stringstream ss;
        ss << std::hex << hex.substr(i, 2);
        ss >> byte;
        out.push_back(static_cast<uint8_t>(byte));
    }
    return out;
}

static std::string GetStorageDir() {
    const char* env_dir = getenv("ZONHAND_NVS_DIR");
    if (env_dir != nullptr && env_dir[0] != '\0') {
        return env_dir;
    }
    const char* home = getenv("HOME");
    if (home != nullptr && home[0] != '\0') {
        return std::string(home) + "/.zOnhand_sim/nvs";
    }
    return "./.zOnhand_sim/nvs";
}

static void EnsureDir(const std::string& dir) {
    std::string tmp = dir;
    for (size_t i = 1; i < tmp.size(); ++i) {
        if (tmp[i] == '/') {
            tmp[i] = '\0';
            ::mkdir(tmp.c_str(), 0755);
            tmp[i] = '/';
        }
    }
    ::mkdir(tmp.c_str(), 0755);
}

static std::string GetNsFilePath(const std::string& ns) {
    std::string dir = GetStorageDir();
    EnsureDir(dir);
    return dir + "/" + ns + ".dat";
}

static void LoadNamespaceLocked(const std::string& ns) {
    if (s_namespaces.find(ns) != s_namespaces.end()) {
        return; // Already in memory
    }
    auto& entries = s_namespaces[ns];
    std::string path = GetNsFilePath(ns);
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        return;
    }
    std::string line;
    while (std::getline(ifs, line)) {
        if (line.size() < 4 || line[1] != ':') {
            continue;
        }
        char type_c = line[0];
        size_t eq = line.find('=', 2);
        if (eq == std::string::npos) {
            continue;
        }
        std::string key = line.substr(2, eq - 2);
        std::string val = line.substr(eq + 1);

        NvsEntry e;
        if (type_c == 'I') {
            e.type = NvsType::kInt64;
            e.num_val = std::stoll(val);
        } else if (type_c == 'S') {
            e.type = NvsType::kString;
            e.str_val = val;
        } else if (type_c == 'B') {
            e.type = NvsType::kBlob;
            e.blob_val = HexDecode(val);
        } else {
            continue;
        }
        entries[key] = std::move(e);
    }
}

static void SaveNamespaceLocked(const std::string& ns) {
    auto it = s_namespaces.find(ns);
    if (it == s_namespaces.end()) {
        return;
    }
    std::string path = GetNsFilePath(ns);
    std::string tmp_path = path + ".tmp";
    std::ofstream ofs(tmp_path, std::ios::trunc);
    if (!ofs.is_open()) {
        return;
    }
    for (const auto& kv : it->second) {
        const std::string& key = kv.first;
        const NvsEntry& e = kv.second;
        if (e.type == NvsType::kInt64) {
            ofs << "I:" << key << "=" << e.num_val << "\n";
        } else if (e.type == NvsType::kString) {
            ofs << "S:" << key << "=" << e.str_val << "\n";
        } else if (e.type == NvsType::kBlob) {
            ofs << "B:" << key << "=" << HexEncode(e.blob_val.data(), e.blob_val.size()) << "\n";
        }
    }
    ofs.close();
    ::rename(tmp_path.c_str(), path.c_str());
}

template<typename T>
static esp_err_t SetInteger(nvs_handle_t handle, const char* key, T val) {
    if (key == nullptr) return ESP_ERR_INVALID_ARG;
    std::lock_guard<std::mutex> lock(s_nvs_mutex);
    auto it = s_handles.find(handle);
    if (it == s_handles.end()) return ESP_ERR_NVS_INVALID_HANDLE;
    if (it->second.mode == NVS_READONLY) return ESP_ERR_NVS_READ_ONLY;

    NvsEntry e;
    e.type = NvsType::kInt64;
    e.num_val = static_cast<int64_t>(val);
    s_namespaces[it->second.ns_name][key] = std::move(e);
    return ESP_OK;
}

template<typename T>
static esp_err_t GetInteger(nvs_handle_t handle, const char* key, T* out_val) {
    if (key == nullptr || out_val == nullptr) return ESP_ERR_INVALID_ARG;
    std::lock_guard<std::mutex> lock(s_nvs_mutex);
    auto it = s_handles.find(handle);
    if (it == s_handles.end()) return ESP_ERR_NVS_INVALID_HANDLE;

    auto& ns_map = s_namespaces[it->second.ns_name];
    auto entry_it = ns_map.find(key);
    if (entry_it == ns_map.end()) {
        return ESP_ERR_NVS_NOT_FOUND;
    }
    if (entry_it->second.type != NvsType::kInt64) {
        return ESP_ERR_NVS_TYPE_MISMATCH;
    }
    *out_val = static_cast<T>(entry_it->second.num_val);
    return ESP_OK;
}

}  // namespace

extern "C" {

esp_err_t nvs_flash_init(void) {
    std::lock_guard<std::mutex> lock(s_nvs_mutex);
    s_initialized = true;
    EnsureDir(GetStorageDir());
    return ESP_OK;
}

esp_err_t nvs_flash_erase(void) {
    std::lock_guard<std::mutex> lock(s_nvs_mutex);
    s_namespaces.clear();
    // In a directory, remove .dat files
    std::string dir = GetStorageDir();
    // System command or posix remove
    std::string cmd = "rm -f \"" + dir + "\"/*.dat \"" + dir + "\"/*.tmp 2>/dev/null";
    (void)::system(cmd.c_str());
    return ESP_OK;
}

esp_err_t nvs_flash_deinit(void) {
    std::lock_guard<std::mutex> lock(s_nvs_mutex);
    s_initialized = false;
    s_handles.clear();
    return ESP_OK;
}

esp_err_t nvs_open(const char* name, nvs_open_mode_t open_mode, nvs_handle_t *out_handle) {
    if (name == nullptr || out_handle == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    std::lock_guard<std::mutex> lock(s_nvs_mutex);
    if (!s_initialized) {
        s_initialized = true;
        EnsureDir(GetStorageDir());
    }
    LoadNamespaceLocked(name);
    nvs_handle_t h = s_next_handle++;
    s_handles[h] = NvsHandleContext{name, open_mode};
    *out_handle = h;
    return ESP_OK;
}

void nvs_close(nvs_handle_t handle) {
    std::lock_guard<std::mutex> lock(s_nvs_mutex);
    s_handles.erase(handle);
}

esp_err_t nvs_commit(nvs_handle_t handle) {
    std::lock_guard<std::mutex> lock(s_nvs_mutex);
    auto it = s_handles.find(handle);
    if (it == s_handles.end()) {
        return ESP_ERR_NVS_INVALID_HANDLE;
    }
    SaveNamespaceLocked(it->second.ns_name);
    return ESP_OK;
}

// ---------------------------------------------------------------------------
// Setters
// ---------------------------------------------------------------------------

esp_err_t nvs_set_i8(nvs_handle_t handle, const char* key, int8_t value) {
    return SetInteger(handle, key, value);
}

esp_err_t nvs_set_u8(nvs_handle_t handle, const char* key, uint8_t value) {
    return SetInteger(handle, key, value);
}

esp_err_t nvs_set_i16(nvs_handle_t handle, const char* key, int16_t value) {
    return SetInteger(handle, key, value);
}

esp_err_t nvs_set_u16(nvs_handle_t handle, const char* key, uint16_t value) {
    return SetInteger(handle, key, value);
}

esp_err_t nvs_set_i32(nvs_handle_t handle, const char* key, int32_t value) {
    return SetInteger(handle, key, value);
}

esp_err_t nvs_set_u32(nvs_handle_t handle, const char* key, uint32_t value) {
    return SetInteger(handle, key, value);
}

esp_err_t nvs_set_i64(nvs_handle_t handle, const char* key, int64_t value) {
    return SetInteger(handle, key, value);
}

esp_err_t nvs_set_u64(nvs_handle_t handle, const char* key, uint64_t value) {
    return SetInteger(handle, key, static_cast<int64_t>(value));
}

esp_err_t nvs_set_str(nvs_handle_t handle, const char* key, const char* value) {
    if (key == nullptr || value == nullptr) return ESP_ERR_INVALID_ARG;
    std::lock_guard<std::mutex> lock(s_nvs_mutex);
    auto it = s_handles.find(handle);
    if (it == s_handles.end()) return ESP_ERR_NVS_INVALID_HANDLE;
    if (it->second.mode == NVS_READONLY) return ESP_ERR_NVS_READ_ONLY;

    NvsEntry e;
    e.type = NvsType::kString;
    e.str_val = value;
    s_namespaces[it->second.ns_name][key] = std::move(e);
    return ESP_OK;
}

esp_err_t nvs_set_blob(nvs_handle_t handle, const char* key, const void* value, size_t length) {
    if (key == nullptr || (value == nullptr && length > 0)) return ESP_ERR_INVALID_ARG;
    std::lock_guard<std::mutex> lock(s_nvs_mutex);
    auto it = s_handles.find(handle);
    if (it == s_handles.end()) return ESP_ERR_NVS_INVALID_HANDLE;
    if (it->second.mode == NVS_READONLY) return ESP_ERR_NVS_READ_ONLY;

    NvsEntry e;
    e.type = NvsType::kBlob;
    const uint8_t* p = static_cast<const uint8_t*>(value);
    e.blob_val.assign(p, p + length);
    s_namespaces[it->second.ns_name][key] = std::move(e);
    return ESP_OK;
}

// ---------------------------------------------------------------------------
// Getters
// ---------------------------------------------------------------------------

esp_err_t nvs_get_i8(nvs_handle_t handle, const char* key, int8_t* out_value) {
    return GetInteger(handle, key, out_value);
}

esp_err_t nvs_get_u8(nvs_handle_t handle, const char* key, uint8_t* out_value) {
    return GetInteger(handle, key, out_value);
}

esp_err_t nvs_get_i16(nvs_handle_t handle, const char* key, int16_t* out_value) {
    return GetInteger(handle, key, out_value);
}

esp_err_t nvs_get_u16(nvs_handle_t handle, const char* key, uint16_t* out_value) {
    return GetInteger(handle, key, out_value);
}

esp_err_t nvs_get_i32(nvs_handle_t handle, const char* key, int32_t* out_value) {
    return GetInteger(handle, key, out_value);
}

esp_err_t nvs_get_u32(nvs_handle_t handle, const char* key, uint32_t* out_value) {
    return GetInteger(handle, key, out_value);
}

esp_err_t nvs_get_i64(nvs_handle_t handle, const char* key, int64_t* out_value) {
    return GetInteger(handle, key, out_value);
}

esp_err_t nvs_get_u64(nvs_handle_t handle, const char* key, uint64_t* out_value) {
    return GetInteger(handle, key, out_value);
}

esp_err_t nvs_get_str(nvs_handle_t handle, const char* key, char* out_value, size_t* length) {
    if (key == nullptr || length == nullptr) return ESP_ERR_INVALID_ARG;
    std::lock_guard<std::mutex> lock(s_nvs_mutex);
    auto it = s_handles.find(handle);
    if (it == s_handles.end()) return ESP_ERR_NVS_INVALID_HANDLE;

    auto& ns_map = s_namespaces[it->second.ns_name];
    auto entry_it = ns_map.find(key);
    if (entry_it == ns_map.end()) {
        return ESP_ERR_NVS_NOT_FOUND;
    }
    if (entry_it->second.type != NvsType::kString) {
        return ESP_ERR_NVS_TYPE_MISMATCH;
    }

    size_t req_len = entry_it->second.str_val.size() + 1;
    if (out_value == nullptr) {
        *length = req_len;
        return ESP_OK;
    }
    if (*length < req_len) {
        *length = req_len;
        return ESP_ERR_NVS_INVALID_LENGTH;
    }
    std::memcpy(out_value, entry_it->second.str_val.c_str(), req_len);
    *length = req_len;
    return ESP_OK;
}

esp_err_t nvs_get_blob(nvs_handle_t handle, const char* key, void* out_value, size_t* length) {
    if (key == nullptr || length == nullptr) return ESP_ERR_INVALID_ARG;
    std::lock_guard<std::mutex> lock(s_nvs_mutex);
    auto it = s_handles.find(handle);
    if (it == s_handles.end()) return ESP_ERR_NVS_INVALID_HANDLE;

    auto& ns_map = s_namespaces[it->second.ns_name];
    auto entry_it = ns_map.find(key);
    if (entry_it == ns_map.end()) {
        return ESP_ERR_NVS_NOT_FOUND;
    }
    if (entry_it->second.type != NvsType::kBlob) {
        return ESP_ERR_NVS_TYPE_MISMATCH;
    }

    size_t req_len = entry_it->second.blob_val.size();
    if (out_value == nullptr) {
        *length = req_len;
        return ESP_OK;
    }
    if (*length < req_len) {
        *length = req_len;
        return ESP_ERR_NVS_INVALID_LENGTH;
    }
    std::memcpy(out_value, entry_it->second.blob_val.data(), req_len);
    *length = req_len;
    return ESP_OK;
}

// ---------------------------------------------------------------------------
// Erase
// ---------------------------------------------------------------------------

esp_err_t nvs_erase_key(nvs_handle_t handle, const char* key) {
    if (key == nullptr) return ESP_ERR_INVALID_ARG;
    std::lock_guard<std::mutex> lock(s_nvs_mutex);
    auto it = s_handles.find(handle);
    if (it == s_handles.end()) return ESP_ERR_NVS_INVALID_HANDLE;
    if (it->second.mode == NVS_READONLY) return ESP_ERR_NVS_READ_ONLY;

    auto& ns_map = s_namespaces[it->second.ns_name];
    ns_map.erase(key);
    return ESP_OK;
}

esp_err_t nvs_erase_all(nvs_handle_t handle) {
    std::lock_guard<std::mutex> lock(s_nvs_mutex);
    auto it = s_handles.find(handle);
    if (it == s_handles.end()) return ESP_ERR_NVS_INVALID_HANDLE;
    if (it->second.mode == NVS_READONLY) return ESP_ERR_NVS_READ_ONLY;

    auto& ns_map = s_namespaces[it->second.ns_name];
    ns_map.clear();
    return ESP_OK;
}

}  // extern "C"
