#pragma once

#include <filesystem>
#include <map>

namespace pf {

class FileWatcher {
public:
    explicit FileWatcher(std::filesystem::path root);

    bool poll(float deltaSeconds);

private:
    std::map<std::filesystem::path, std::filesystem::file_time_type> snapshot() const;

    std::filesystem::path root;
    std::map<std::filesystem::path, std::filesystem::file_time_type> lastSeen;
    float sinceLastCheck = 0;
};

}
