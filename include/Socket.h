#pragma once

#include <cstdint>
#include <string>
#include <stdexcept>

namespace netconnect {

/**
 * @class Socket
 * @brief RAII wrapper around a POSIX TCP socket file descriptor.
 *
 * Owns exactly one OS socket descriptor. The descriptor is closed
 * automatically when the Socket is destroyed, so callers never need to
 * remember to call close() themselves. Socket is move-only: copying a
 * live file descriptor would create two owners for the same OS
 * resource, which is exactly the bug RAII is meant to prevent.
 */
class Socket {
public:
    Socket() = default;

    /// Takes ownership of an already-open descriptor (e.g. from accept()).
    explicit Socket(int fd) noexcept : fd_(fd) {}

    ~Socket();

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    Socket(Socket&& other) noexcept;
    Socket& operator=(Socket&& other) noexcept;

    /// Creates an IPv4 TCP socket. Throws std::runtime_error on failure.
    static Socket createTcp();

    void bindTo(const std::string& address, uint16_t port);
    void listenOn(int backlog);

    /// Blocks until a client connects; returns a Socket owning the new fd.
    Socket accept(std::string& outPeerAddress) const;

    void connectTo(const std::string& address, uint16_t port);

    /// Sends the full contents of data, looping over partial writes.
    /// Returns false if the peer closed the connection or on error.
    bool sendAll(const std::string& data) const;

    /// Reads up to bufferSize bytes. Returns empty string on orderly
    /// shutdown by the peer, throws on hard error.
    std::string receiveOnce(size_t bufferSize = 4096) const;

    void closeSocket();

    int fd() const noexcept { return fd_; }
    bool isValid() const noexcept { return fd_ >= 0; }

private:
    int fd_ = -1;
};

} // namespace netconnect
