# NetConnect — Multi-Client TCP Chat System

A multi-threaded, multi-client TCP chat server and client written in
modern C++17 using raw POSIX sockets. Built to demonstrate networking
fundamentals, object-oriented design, and safe concurrency without
relying on any third-party networking library.

## Overview

NetConnect is a terminal-based chat application. One server process
accepts connections from many concurrent clients, each handled on its
own thread. Clients can broadcast messages to everyone, send private
messages to a specific user, list who's online, and disconnect
gracefully. Every event — connections, disconnections, messages, and
errors — is logged to disk with a timestamp.

The project is intentionally built on nothing but the C++ standard
library and POSIX sockets, so the networking and concurrency are fully
visible rather than hidden behind a framework.

## Architecture

```
                        ┌────────────────────┐
                        │   NetConnect Server │
                        └──────────┬───────────┘
                                   │ TCP
              ┌────────────────────┼────────────────────┐
      ┌───────▼───────┐    ┌───────▼───────┐    ┌───────▼───────┐
      │   Client A     │    │   Client B     │    │   Client C     │
      └────────────────┘    └────────────────┘    └────────────────┘
```

Each accepted client runs on its own `std::thread`, blocked on `recv()`
for that client's socket alone. Shared state (the online-user table) is
protected by a `std::mutex`. See [`docs/architecture.md`](docs/architecture.md)
for the full breakdown of the socket flow, thread model, and class
design.

## Features

- Concurrent multi-client chat over TCP, one thread per connection
- Username handshake on connect, with duplicate/invalid-name rejection
- Public broadcast: `User X joined`, `User X left`, and chat messages
- Private messaging: `/msg <username> <message>`
- Online user list: `/users`
- In-client help: `/help`
- Graceful client disconnect: `/quit`
- Server console shows timestamp, IP address, username, and message
  for every event
- Persistent, timestamped logging to `logs/server.log`
- Configurable via `config.txt` (port, max clients, log path, server name)
- Colored terminal output for log levels (info/warning/error)
- Live server statistics (total clients connected, messages relayed)
- Graceful shutdown on `Ctrl+C` (`SIGINT`) — notifies and disconnects
  every client cleanly before exiting

## Networking Concepts Demonstrated

- Raw POSIX TCP sockets: `socket()`, `bind()`, `listen()`, `accept()`,
  `connect()`, `send()`, `recv()`, `close()`
- IPv4 addressing (`AF_INET`, `sockaddr_in`), `inet_pton`/`inet_ntop`
- TCP as a byte stream: newline-delimited message framing
- `SO_REUSEADDR` for fast server restarts
- Thread-per-connection concurrency with `std::thread`
- Mutex-protected shared state (`std::mutex`, `std::lock_guard`)
- Graceful connection teardown and orderly-shutdown detection (`recv() == 0`)

## Folder Structure

```
NetConnect/
├── CMakeLists.txt        # Build configuration (server, client, tests)
├── config.txt             # Default runtime configuration
├── README.md
├── LICENSE
├── docs/
│   ├── architecture.md    # Socket flow, thread model, class diagram
│   └── screenshots.md     # Screenshot placeholders
├── include/                # Public headers (one class per file)
│   ├── Server.h
│   ├── Client.h
│   ├── Socket.h
│   ├── User.h
│   ├── Logger.h
│   ├── Config.h
│   └── Utils.h
├── src/                     # Implementations
│   ├── main_server.cpp
│   ├── main_client.cpp
│   ├── Server.cpp
│   ├── Client.cpp
│   ├── Socket.cpp
│   ├── Logger.cpp
│   ├── Config.cpp
│   └── Utils.cpp
├── tests/                   # CTest unit tests
│   ├── CMakeLists.txt
│   └── test_utils.cpp
├── logs/                     # Runtime log output (created automatically)
└── screenshots/               # Screenshots referenced from docs/screenshots.md
```

## Installation

Requirements:

- A POSIX system (Linux or macOS)
- CMake 3.15+
- A C++17 compiler (GCC 9+, Clang 10+, or Apple Clang equivalent)

```bash
git clone https://github.com/<your-username>/NetConnect.git
cd NetConnect
```

## Compilation

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

This produces two executables inside `build/`: `netconnect_server` and
`netconnect_client`. The default `config.txt` is copied alongside them.

To build and run the unit tests:

```bash
cmake --build . --target test_utils
ctest --output-on-failure
```

## Usage

### Start the server

From the `build/` directory:

```bash
./netconnect_server ../config.txt
```

Or from the project root, pointing at the top-level config:

```bash
./build/netconnect_server config.txt
```

The server prints a startup line once it's listening, and keeps running
until you stop it with `Ctrl+C` (SIGINT), which triggers a graceful
shutdown — every connected client is notified and disconnected cleanly.

### Connect a client

In a separate terminal:

```bash
./build/netconnect_client 127.0.0.1 54000
```

You'll be prompted for a username, then dropped into the chat session.

### In-chat commands

| Command                  | Description                          |
|---------------------------|---------------------------------------|
| `/msg <username> <text>`  | Send a private message to one user    |
| `/users`                   | List all currently online users        |
| `/help`                    | Show the list of supported commands    |
| `/quit`                    | Disconnect gracefully                  |

Anything typed that isn't a command is broadcast to every other
connected client.

### Configuration (`config.txt`)

```ini
port=54000
max_clients=100
log_path=logs/server.log
server_name=NetConnect Server
```

## Screenshots

See [`docs/screenshots.md`](docs/screenshots.md) for placeholders —
add your own captures to the `screenshots/` folder as you run the
project.

## Future Improvements

- Replace thread-per-connection with an `epoll`/`kqueue`-based event
  loop to scale beyond a few thousand concurrent connections
- TLS support for encrypted client-server traffic
- Message persistence / chat history replay on reconnect
- Chat rooms / channels instead of a single global broadcast
- A length-prefixed binary framing protocol instead of newline-delimited text
- Automated integration tests that spin up a real server and drive it
  with socket-level test clients

## License

Released under the [MIT License](LICENSE).
