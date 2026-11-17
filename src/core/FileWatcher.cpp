#include "core/FileWatcher.hpp"

namespace pf {

namespace {

constexpr float checkInterval = 0.5f;

}

FileWatcher::FileWatcher(std::filesystem::path root) : root(std::move(root)), lastSeen(snapshot()) {}

bool FileWatcher::poll(float deltaSeconds) {
    sinceLastCheck += deltaSeconds;
    if (sinceLastCheck < checkInterval) {
        return false;
    }
    sinceLastCheck = 0;
    auto current = snapshot();
    if (current == lastSeen) {
        return false;
    }
    lastSeen = std::move(current);
    return true;
}

std::map<std::filesystem::path, std::filesystem::file_time_type> FileWatcher::snapshot() const {
    std::map<std::filesystem::path, std::filesystem::file_time_type> times;
    std::error_code error;
    if (std::filesystem::is_regular_file(root, error)) {
        times[root] = std::filesystem::last_write_time(root, error);
        return times;
    }
    for (const auto& entry : std::filesystem::recursive_directory_iterator(root, error)) {
        if (entry.is_regular_file(error)) {
            times[entry.path()] = entry.last_write_time(error);
        }
    }
    return times;
}

}
