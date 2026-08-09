#include "Utils.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <sstream>

namespace netconnect::Utils {

std::string currentTimestamp() {
    using namespace std::chrono;
    const auto now = system_clock::now();
    const std::time_t t = system_clock::to_time_t(now);

    std::tm localTm{};
    localtime_r(&t, &localTm); // POSIX thread-safe variant of localtime()

    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &localTm);
    return std::string(buf);
}

std::vector<std::string> split(const std::string& text, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(text);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

std::string trim(const std::string& text) {
    auto start = std::find_if_not(text.begin(), text.end(),
                                   [](unsigned char c) { return std::isspace(c); });
    auto end = std::find_if_not(text.rbegin(), text.rend(),
                                 [](unsigned char c) { return std::isspace(c); }).base();
    return (start < end) ? std::string(start, end) : std::string();
}

bool isValidUsername(const std::string& username) {
    if (username.size() < 2 || username.size() > 20) return false;
    return std::all_of(username.begin(), username.end(), [](unsigned char c) {
        return std::isalnum(c) || c == '_';
    });
}

} // namespace netconnect::Utils
