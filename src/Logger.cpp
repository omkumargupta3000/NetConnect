#include "Logger.h"

#include <filesystem>
#include <iostream>

#include "Utils.h"

namespace netconnect {

namespace {
constexpr const char* kColorReset = "\033[0m";
constexpr const char* kColorGreen = "\033[32m";
constexpr const char* kColorYellow = "\033[33m";
constexpr const char* kColorRed = "\033[31m";
} // namespace

Logger::Logger(const std::string& logFilePath) {
    std::filesystem::path path(logFilePath);
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path());
    }
    file_.open(logFilePath, std::ios::app);
    if (!file_.is_open()) {
        std::cerr << "Warning: could not open log file at " << logFilePath
                  << ", logging to console only.\n";
    }
}

Logger::~Logger() {
    if (file_.is_open()) {
        file_.flush();
        file_.close();
    }
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Info: return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error: return "ERROR";
    }
    return "INFO";
}

std::string Logger::colorFor(LogLevel level) {
    switch (level) {
        case LogLevel::Info: return kColorGreen;
        case LogLevel::Warning: return kColorYellow;
        case LogLevel::Error: return kColorRed;
    }
    return kColorReset;
}

void Logger::log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);

    const std::string timestamp = Utils::currentTimestamp();
    const std::string levelStr = levelToString(level);
    const std::string line = "[" + timestamp + "] [" + levelStr + "] " + message;

    std::cout << colorFor(level) << line << kColorReset << std::endl;

    if (file_.is_open()) {
        file_ << line << std::endl;
    }
}

} // namespace netconnect
