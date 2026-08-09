#pragma once

#include <memory>
#include <mutex>
#include <string>

#include "Socket.h"

namespace netconnect {

/**
 * @class User
 * @brief Represents a single connected chat client on the server side.
 *
 * Bundles the identity (username, IP) of a client together with the
 * Socket used to reach them. The socket is guarded by a mutex because
 * the server's broadcast path and a client's own handler thread can
 * both attempt to write to it concurrently.
 */
class User {
public:
    User(std::string username, std::string ipAddress, Socket socket)
        : username_(std::move(username)),
          ipAddress_(std::move(ipAddress)),
          socket_(std::move(socket)) {}

    const std::string& username() const noexcept { return username_; }
    const std::string& ipAddress() const noexcept { return ipAddress_; }

    /// Thread-safe send: serializes concurrent writers on the same socket.
    bool send(const std::string& message) {
        std::lock_guard<std::mutex> lock(sendMutex_);
        return socket_.sendAll(message);
    }

    Socket& socket() noexcept { return socket_; }

private:
    std::string username_;
    std::string ipAddress_;
    Socket socket_;
    std::mutex sendMutex_;
};

using UserPtr = std::shared_ptr<User>;

} // namespace netconnect
