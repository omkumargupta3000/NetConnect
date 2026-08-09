#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "Config.h"
#include "Logger.h"
#include "Socket.h"
#include "User.h"

namespace netconnect {

/**
 * @class Server
 * @brief Multi-client TCP chat server.
 *
 * Owns a listening Socket and, for each connected client, spawns a
 * dedicated std::thread that blocks on recv() for that client alone.
 * Shared state (the user table and running stats) is protected by a
 * mutex since it is read and written concurrently by every client
 * thread. This is the classic "thread-per-connection" model: simple to
 * reason about and a good fit for a chat server where connection counts
 * are modest and each connection is mostly idle waiting on I/O.
 */
class Server {
public:
    explicit Server(Config config);
    ~Server();

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    /// Binds, listens, and blocks accepting clients until stop() is called.
    void run();

    /// Signals the accept loop and all client threads to shut down.
    void stop();

private:
    void acceptLoop();
    void handleClient(Socket clientSocket, std::string ipAddress);

    // --- Command handling ---
    // Returns false when the session should end (e.g. after /quit), so
    // the caller can stop reading from a socket it just closed instead
    // of looping back into recv() on a dead file descriptor.
    bool processMessage(const UserPtr& sender, const std::string& rawLine);
    void handlePrivateMessage(const UserPtr& sender, const std::string& args);
    void handleListUsers(const UserPtr& sender);
    void handleHelp(const UserPtr& sender);

    // --- Broadcasting ---
    void broadcast(const std::string& message, const std::string& excludeUsername = "");
    void removeUser(const std::string& username);

    std::string registerUser(const UserPtr& user); // returns rejection reason, or "" on success

    Config config_;
    Logger logger_;
    Socket listenSocket_;

    std::atomic<bool> running_{false};

    std::mutex usersMutex_;
    std::unordered_map<std::string, UserPtr> users_;

    // --- Statistics (bonus feature) ---
    std::atomic<uint64_t> totalClientsConnected_{0};
    std::atomic<uint64_t> totalMessagesRelayed_{0};
};

} // namespace netconnect
