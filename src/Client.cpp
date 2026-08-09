#include "Client.h"

#include <iostream>
#include <thread>

namespace netconnect {

Client::Client(std::string serverAddress, uint16_t port, Logger& logger)
    : serverAddress_(std::move(serverAddress)), port_(port), logger_(logger) {}

void Client::run() {
    try {
        socket_ = Socket::createTcp();
        socket_.connectTo(serverAddress_, port_);
    } catch (const std::exception& e) {
        logger_.error(std::string("Could not connect: ") + e.what());
        return;
    }

    connected_ = true;
    logger_.info("Connected to " + serverAddress_ + ":" + std::to_string(port_));

    // Receiver thread owns all reads from the socket; the main thread
    // below owns all writes, so the two never race on the same syscall.
    std::thread receiver(&Client::receiveLoop, this);

    std::string line;
    while (connected_ && std::getline(std::cin, line)) {
        if (!socket_.sendAll(line + "\n")) {
            std::cout << "[Client] Connection lost.\n";
            break;
        }
        if (line == "/quit") break;
    }

    connected_ = false;
    socket_.closeSocket(); // unblocks receiveLoop()'s recv() call

    if (receiver.joinable()) receiver.join();
    logger_.info("Disconnected.");
}

void Client::receiveLoop() {
    while (connected_) {
        std::string data;
        try {
            data = socket_.receiveOnce();
        } catch (const std::exception&) {
            break; // socket closed from the main thread during shutdown
        }
        if (data.empty()) {
            connected_ = false;
            std::cout << "[Client] Server closed the connection.\n";
            break;
        }
        std::cout << data;
        std::cout.flush();
    }
}

} // namespace netconnect
