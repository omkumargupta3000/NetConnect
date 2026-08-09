#pragma once

#include <atomic>
#include <string>

#include "Logger.h"
#include "Socket.h"

namespace netconnect {

/**
 * @class Client
 * @brief Connects to a NetConnect server and runs an interactive chat
 *        session.
 *
 * Uses two threads: the main thread reads from stdin and sends to the
 * server, while a dedicated receiver thread blocks on recv() and prints
 * incoming messages. Splitting these two directions across threads
 * avoids the classic problem of a single thread being unable to react
 * to an incoming broadcast while it is blocked waiting on user input.
 */
class Client {
public:
    Client(std::string serverAddress, uint16_t port, Logger& logger);

    /// Connects, then blocks running the interactive session until /quit
    /// or the connection is lost.
    void run();

private:
    void receiveLoop();

    std::string serverAddress_;
    uint16_t port_;
    Logger& logger_;
    Socket socket_;
    std::atomic<bool> connected_{false};
};

} // namespace netconnect
