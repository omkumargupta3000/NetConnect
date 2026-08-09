#include "Socket.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace netconnect {

Socket::~Socket() {
    closeSocket();
}

Socket::Socket(Socket&& other) noexcept : fd_(other.fd_) {
    other.fd_ = -1;
}

Socket& Socket::operator=(Socket&& other) noexcept {
    if (this != &other) {
        closeSocket();
        fd_ = other.fd_;
        other.fd_ = -1;
    }
    return *this;
}

Socket Socket::createTcp() {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        throw std::runtime_error(std::string("socket() failed: ") + std::strerror(errno));
    }
    int opt = 1;
    // Allows immediate rebinding to the port after restart instead of
    // waiting out TIME_WAIT; standard practice for TCP servers.
    ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    return Socket(fd);
}

void Socket::bindTo(const std::string& address, uint16_t port) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (address.empty() || address == "0.0.0.0") {
        addr.sin_addr.s_addr = INADDR_ANY;
    } else if (::inet_pton(AF_INET, address.c_str(), &addr.sin_addr) <= 0) {
        throw std::runtime_error("Invalid bind address: " + address);
    }

    if (::bind(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        throw std::runtime_error(std::string("bind() failed: ") + std::strerror(errno));
    }
}

void Socket::listenOn(int backlog) {
    if (::listen(fd_, backlog) < 0) {
        throw std::runtime_error(std::string("listen() failed: ") + std::strerror(errno));
    }
}

Socket Socket::accept(std::string& outPeerAddress) const {
    sockaddr_in clientAddr{};
    socklen_t clientLen = sizeof(clientAddr);

    int clientFd = ::accept(fd_, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
    if (clientFd < 0) {
        throw std::runtime_error(std::string("accept() failed: ") + std::strerror(errno));
    }

    char ipBuf[INET_ADDRSTRLEN] = {0};
    ::inet_ntop(AF_INET, &clientAddr.sin_addr, ipBuf, sizeof(ipBuf));
    outPeerAddress = ipBuf;

    return Socket(clientFd);
}

void Socket::connectTo(const std::string& address, uint16_t port) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (::inet_pton(AF_INET, address.c_str(), &addr.sin_addr) <= 0) {
        throw std::runtime_error("Invalid server address: " + address);
    }

    if (::connect(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        throw std::runtime_error(std::string("connect() failed: ") + std::strerror(errno));
    }
}

bool Socket::sendAll(const std::string& data) const {
    size_t totalSent = 0;
    const char* buf = data.data();
    const size_t len = data.size();

    while (totalSent < len) {
        ssize_t sent = ::send(fd_, buf + totalSent, len - totalSent, 0);
        if (sent < 0) {
            if (errno == EINTR) continue; // interrupted by signal, retry
            return false; // broken pipe / connection reset / etc.
        }
        if (sent == 0) return false;
        totalSent += static_cast<size_t>(sent);
    }
    return true;
}

std::string Socket::receiveOnce(size_t bufferSize) const {
    std::string buffer(bufferSize, '\0');
    ssize_t received = ::recv(fd_, buffer.data(), bufferSize, 0);

    if (received < 0) {
        throw std::runtime_error(std::string("recv() failed: ") + std::strerror(errno));
    }
    if (received == 0) {
        return std::string(); // peer performed an orderly shutdown
    }
    buffer.resize(static_cast<size_t>(received));
    return buffer;
}

void Socket::closeSocket() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

} // namespace netconnect
