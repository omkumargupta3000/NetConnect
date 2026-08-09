#pragma once

#include <cstdint>
#include <string>

namespace netconnect {

/**
 * @class Config
 * @brief Loads server configuration from a simple "key=value" text file.
 *
 * Keeps configuration out of source code so the server's port, capacity,
 * log location and display name can be changed without recompiling.
 * Falls back to sane defaults for any key that is missing, so a
 * malformed or partial config.txt never prevents the server from
 * starting.
 */
class Config {
public:
    /// Loads from the given path; missing file or keys fall back to defaults.
    static Config loadFromFile(const std::string& path);

    uint16_t port() const noexcept { return port_; }
    int maxClients() const noexcept { return maxClients_; }
    const std::string& logPath() const noexcept { return logPath_; }
    const std::string& serverName() const noexcept { return serverName_; }

private:
    uint16_t port_ = 54000;
    int maxClients_ = 100;
    std::string logPath_ = "logs/server.log";
    std::string serverName_ = "NetConnect Server";
};

} // namespace netconnect
