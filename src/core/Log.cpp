#include "core/Log.hpp"

#include <SDL3/SDL_log.h>

namespace pf {

namespace {

constexpr std::size_t maxEntries = 200;

std::deque<LogEntry>& storage() {
    static std::deque<LogEntry> entries;
    return entries;
}

}

void Log::info(std::string message) { push(LogLevel::Info, std::move(message)); }

void Log::warning(std::string message) { push(LogLevel::Warning, std::move(message)); }

void Log::error(std::string message) { push(LogLevel::Error, std::move(message)); }

const std::deque<LogEntry>& Log::entries() { return storage(); }

void Log::push(LogLevel level, std::string message) {
    SDL_Log("%s", message.c_str());
    auto& entries = storage();
    entries.push_back({level, std::move(message)});
    if (entries.size() > maxEntries) {
        entries.pop_front();
    }
}

}
