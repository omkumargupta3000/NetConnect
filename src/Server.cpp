#include "Server.h"

#include <iostream>
#include <sstream>

#include "Utils.h"

namespace netconnect {

namespace {
constexpr int kListenBacklog = 64;
constexpr const char* kHelpText =
    "Available commands:\n"
    "  /msg <username> <message>  - send a private message\n"
    "  /users                     - list online users\n"
    "  /help                      - show this help text\n"
    "  /quit                      - disconnect from the server\n";
} // namespace

Server::Server(Config config)
    : config_(std::move(config)), logger_(config_.logPath()) {}

Server::~Server() {
    stop();
}

void Server::run() {
    listenSocket_ = Socket::createTcp();
    listenSocket_.bindTo("0.0.0.0", config_.port());
    listenSocket_.listenOn(kListenBacklog);

    running_ = true;
    logger_.info(config_.serverName() + " listening on port " +
                 std::to_string(config_.port()) +
                 " (max_clients=" + std::to_string(config_.maxClients()) + ")");

    acceptLoop();
}

void Server::stop() {
    if (!running_.exchange(false)) {
        return; // already stopped
    }
    logger_.info("Server shutting down...");
    listenSocket_.closeSocket(); // unblocks the accept() call in acceptLoop()

    // Notify and disconnect any still-connected clients.
    std::lock_guard<std::mutex> lock(usersMutex_);
    for (auto& [name, user] : users_) {
        user->send("[Server] Shutting down. Goodbye!\n");
        user->socket().closeSocket();
    }
    users_.clear();
}

void Server::acceptLoop() {
    while (running_) {
        std::string peerIp;
        Socket clientSocket;
        try {
            clientSocket = listenSocket_.accept(peerIp);
        } catch (const std::exception& e) {
            if (running_) {
                logger_.error(std::string("accept() error: ") + e.what());
            }
            break; // listen socket was closed (shutdown) or genuinely failed
        }

        // Thread-per-connection: each client gets its own thread blocked
        // on recv() so one slow/idle client never stalls the others.
        // The thread is detached because its lifetime is naturally
        // bounded by the client's connection, and the server doesn't
        // need to join on it individually - stop() handles shutdown by
        // closing sockets, which unblocks each thread's recv() call.
        std::thread(&Server::handleClient, this, std::move(clientSocket), peerIp).detach();
    }
}

void Server::handleClient(Socket clientSocket, std::string ipAddress) {
    // --- Username negotiation ---
    UserPtr user;
    while (running_) {
        if (!clientSocket.sendAll("Enter username: ")) return;

        std::string data;
        try {
            data = clientSocket.receiveOnce();
        } catch (const std::exception& e) {
            logger_.error("recv() error during handshake from " + ipAddress + ": " + e.what());
            return;
        }
        if (data.empty()) return; // client disconnected before choosing a name

        std::string candidate = Utils::trim(data);
        if (!Utils::isValidUsername(candidate)) {
            clientSocket.sendAll("Invalid username. Use 2-20 alphanumeric characters or '_'.\n");
            continue;
        }

        auto newUser = std::make_shared<User>(candidate, ipAddress, std::move(clientSocket));
        std::string rejection = registerUser(newUser);
        if (!rejection.empty()) {
            newUser->send(rejection + "\n");
            clientSocket = std::move(newUser->socket()); // reclaim socket, try again
            continue;
        }

        user = newUser;
        break;
    }
    if (!user) return;

    totalClientsConnected_++;
    logger_.info("Client connected: " + user->username() + " (" + user->ipAddress() + ")");
    user->send("Welcome to " + config_.serverName() + ", " + user->username() + "!\n" + kHelpText);
    broadcast("User " + user->username() + " joined", user->username());

    // --- Message loop ---
    while (running_) {
        std::string data;
        try {
            data = user->socket().receiveOnce();
        } catch (const std::exception& e) {
            logger_.error("recv() error for " + user->username() + ": " + e.what());
            break;
        }
        if (data.empty()) break; // orderly disconnect or broken pipe

        std::istringstream stream(data);
        std::string line;
        bool keepGoing = true;
        while (keepGoing && std::getline(stream, line)) {
            line = Utils::trim(line);
            if (!line.empty()) {
                keepGoing = processMessage(user, line);
            }
        }
        if (!keepGoing) break;
    }

    removeUser(user->username());
}

std::string Server::registerUser(const UserPtr& user) {
    std::lock_guard<std::mutex> lock(usersMutex_);

    if (static_cast<int>(users_.size()) >= config_.maxClients()) {
        return "[Server] Server is full. Please try again later.";
    }
    if (users_.count(user->username()) > 0) {
        return "[Server] Username '" + user->username() + "' is already taken.";
    }
    users_[user->username()] = user;
    return "";
}

void Server::removeUser(const std::string& username) {
    {
        std::lock_guard<std::mutex> lock(usersMutex_);
        users_.erase(username);
    }
    logger_.info("Client disconnected: " + username);
    broadcast("User " + username + " left", username);
}

bool Server::processMessage(const UserPtr& sender, const std::string& rawLine) {
    logger_.info("[" + sender->ipAddress() + "] " + sender->username() + ": " + rawLine);

    if (rawLine.rfind("/msg ", 0) == 0) {
        handlePrivateMessage(sender, rawLine.substr(5));
    } else if (rawLine == "/users") {
        handleListUsers(sender);
    } else if (rawLine == "/help") {
        handleHelp(sender);
    } else if (rawLine == "/quit") {
        sender->send("[Server] Goodbye!\n");
        sender->socket().closeSocket();
        return false; // tell handleClient to stop reading from this socket
    } else if (!rawLine.empty() && rawLine[0] == '/') {
        sender->send("[Server] Unknown command. Type /help for a list of commands.\n");
    } else {
        totalMessagesRelayed_++;
        broadcast(sender->username() + ": " + rawLine, sender->username());
    }
    return true;
}

void Server::handlePrivateMessage(const UserPtr& sender, const std::string& args) {
    auto spacePos = args.find(' ');
    if (spacePos == std::string::npos) {
        sender->send("[Server] Usage: /msg <username> <message>\n");
        return;
    }

    std::string targetName = args.substr(0, spacePos);
    std::string message = args.substr(spacePos + 1);

    std::lock_guard<std::mutex> lock(usersMutex_);
    auto it = users_.find(targetName);
    if (it == users_.end()) {
        sender->send("[Server] User '" + targetName + "' is not online.\n");
        return;
    }

    it->second->send("[PM from " + sender->username() + "] " + message + "\n");
    sender->send("[PM to " + targetName + "] " + message + "\n");
}

void Server::handleListUsers(const UserPtr& sender) {
    std::lock_guard<std::mutex> lock(usersMutex_);

    std::ostringstream out;
    out << "[Server] Online users (" << users_.size() << "):\n";
    for (const auto& [name, user] : users_) {
        out << "  - " << name << "\n";
    }
    sender->send(out.str());
}

void Server::handleHelp(const UserPtr& sender) {
    sender->send(std::string("[Server]\n") + kHelpText);
}

void Server::broadcast(const std::string& message, const std::string& excludeUsername) {
    std::lock_guard<std::mutex> lock(usersMutex_);
    const std::string formatted = "[" + Utils::currentTimestamp() + "] " + message + "\n";

    for (const auto& [name, user] : users_) {
        if (name != excludeUsername) {
            user->send(formatted);
        }
    }
}

} // namespace netconnect
