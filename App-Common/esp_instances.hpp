#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <expected>
#include <cerrno>
#include <cstdlib>
#include <unistd.h>
#include <signal.h>

namespace esp_instances {

namespace config {
    constexpr const char* APP_NAME      = "RobloxPlayer";
    constexpr const char* DYLIB_NAME    = "libApp-ESPManager.dylib";
    constexpr const char* INSTANCE_DIR  = "/tmp/esp_instances";
    constexpr const char* SHM_PREFIX    = "esp_shared_memory_";
}

enum class Error {
    DirectoryCreateFailed,
    FileCreateFailed,
    InvalidFilename,
};

inline std::string_view error_string(Error e) {
    switch (e) {
        case Error::DirectoryCreateFailed: return "failed to create instance directory";
        case Error::FileCreateFailed:      return "failed to create shared memory file";
        case Error::InvalidFilename:       return "filename does not contain a valid pid";
    }
    return "unknown error";
}

inline std::filesystem::path shm_path_for_pid(pid_t pid) {
    return std::filesystem::path(config::INSTANCE_DIR) /
           (std::string(config::SHM_PREFIX) + std::to_string(pid));
}

inline bool is_pid_alive(pid_t pid) {
    return ::kill(pid, 0) == 0 || errno == EPERM;
}

[[nodiscard]]
inline std::expected<void, Error> ensure_instance_dir() {
    std::error_code ec;
    std::filesystem::create_directories(config::INSTANCE_DIR, ec);
    if (ec) return std::unexpected(Error::DirectoryCreateFailed);
    return {};
}

inline void unregister_instance(pid_t pid) {
    std::error_code ec;
    std::filesystem::remove(shm_path_for_pid(pid), ec);
}











    // injector


struct InstanceEntry {
    pid_t       pid;
    std::string shm_path;
};

[[nodiscard]]
inline std::expected<InstanceEntry, Error>
parse_instance_entry(const std::filesystem::path& path) {
    const std::string name = path.filename().string();

    if (!name.starts_with(config::SHM_PREFIX))
        return std::unexpected(Error::InvalidFilename);

    const char* pid_start = name.c_str() + std::string_view(config::SHM_PREFIX).size();

    char* end = nullptr;
    errno = 0;
    long val = std::strtol(pid_start, &end, 10);

    if (errno != 0 || end == pid_start || *end != '\0' || val <= 0)
        return std::unexpected(Error::InvalidFilename);

    return InstanceEntry{ static_cast<pid_t>(val), path.string() };
}

[[nodiscard]]
inline std::vector<InstanceEntry> scan_instances() {
    std::vector<InstanceEntry> result;

    std::error_code ec;
    if (!std::filesystem::exists(config::INSTANCE_DIR, ec))
        return result;

    for (const auto& entry : std::filesystem::directory_iterator(config::INSTANCE_DIR, ec)) {
        if (ec)
            break;

        auto parsed = parse_instance_entry(entry.path());
        if (!parsed)
            continue;

        if (!is_pid_alive(parsed->pid)) {
            std::filesystem::remove(entry.path(), ec);
            continue;
        }

        result.push_back(std::move(*parsed));
    }

    return result;
}

} // namespace esp_instances
