#pragma once

#include <deque>
#include <string>

namespace pf {

enum class LogLevel { Info, Warning, Error };

struct LogEntry {
    LogLevel level;
    std::string message;
};

class Log {
public:
    static void info(std::string message);
    static void warning(std::string message);
    static void error(std::string message);
    static const std::deque<LogEntry>& entries();

private:
    static void push(LogLevel level, std::string message);
};

}
