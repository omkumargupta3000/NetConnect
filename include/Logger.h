#pragma once

#include <fstream>
#include <mutex>
#include <string>

namespace netconnect {

enum class LogLevel { Info, Warning, Error };

/**
 * @class Logger
 * @brief Thread-safe logger that writes timestamped entries to a file
 *        and mirrors them to the console with color coding.
 *
 * A single Logger instance is owned by Server/Client and passed by
 * reference to collaborators, rather than exposed as a global/singleton.
 * This keeps dependencies explicit and makes the class unit-testable.
 */
class Logger {
public:
    explicit Logger(const std::string& logFilePath);
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void log(LogLevel level, const std::string& message);

    void info(const std::string& message) { log(LogLevel::Info, message); }
    void warning(const std::string& message) { log(LogLevel::Warning, message); }
    void error(const std::string& message) { log(LogLevel::Error, message); }

private:
    std::ofstream file_;
    std::mutex mutex_;

    static std::string levelToString(LogLevel level);
    static std::string colorFor(LogLevel level);
};

} // namespace netconnect
