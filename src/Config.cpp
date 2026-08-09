#include "Config.h"

#include <fstream>
#include <iostream>
#include <unordered_map>

#include "Utils.h"

namespace netconnect {

Config Config::loadFromFile(const std::string& path) {
    Config config; // start from defaults

    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Warning: config file '" << path
                  << "' not found, using default configuration.\n";
        return config;
    }

    std::unordered_map<std::string, std::string> values;
    std::string line;
    while (std::getline(file, line)) {
        line = Utils::trim(line);
        if (line.empty() || line[0] == '#') continue;

        auto eqPos = line.find('=');
        if (eqPos == std::string::npos) continue;

        std::string key = Utils::trim(line.substr(0, eqPos));
        std::string value = Utils::trim(line.substr(eqPos + 1));
        values[key] = value;
    }

    if (auto it = values.find("port"); it != values.end()) {
        try {
            config.port_ = static_cast<uint16_t>(std::stoi(it->second));
        } catch (const std::exception&) {
            std::cerr << "Warning: invalid 'port' in config, using default.\n";
        }
    }
    if (auto it = values.find("max_clients"); it != values.end()) {
        try {
            config.maxClients_ = std::stoi(it->second);
        } catch (const std::exception&) {
            std::cerr << "Warning: invalid 'max_clients' in config, using default.\n";
        }
    }
    if (auto it = values.find("log_path"); it != values.end()) {
        config.logPath_ = it->second;
    }
    if (auto it = values.find("server_name"); it != values.end()) {
        config.serverName_ = it->second;
    }

    return config;
}

} // namespace netconnect
