#include <csignal>
#include <iostream>

#include "Config.h"
#include "Server.h"

namespace {
// A raw pointer is required here: signal handlers are C callbacks and
// cannot capture state or take arguments, so the server instance is
// reached through a pointer with static storage duration. The handler
// only calls stop(), which is safe to invoke from a signal context.
netconnect::Server* g_server = nullptr;

void handleShutdownSignal(int /*signal*/) {
    if (g_server) {
        g_server->stop();
    }
}
} // namespace

int main(int argc, char* argv[]) {
    std::string configPath = (argc > 1) ? argv[1] : "config.txt";

    netconnect::Config config = netconnect::Config::loadFromFile(configPath);
    netconnect::Server server(config);

    g_server = &server;
    std::signal(SIGINT, handleShutdownSignal);  // Ctrl+C
    std::signal(SIGTERM, handleShutdownSignal); // kill

    try {
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal server error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
