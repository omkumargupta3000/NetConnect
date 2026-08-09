# NetConnect Architecture

## 1. Client-Server Model

NetConnect follows a classic **centralized client-server architecture**:

```
                        ┌────────────────────┐
                        │   NetConnect Server │
                        │  (single process)   │
                        └──────────┬───────────┘
                                   │  TCP :54000
              ┌────────────────────┼────────────────────┐
              │                    │                    │
      ┌───────▼───────┐    ┌───────▼───────┐    ┌───────▼───────┐
      │   Client A     │    │   Client B     │    │   Client C     │
      │  (Alice)       │    │  (Bob)         │    │  (Carol)       │
      └────────────────┘    └────────────────┘    └────────────────┘
```

The server is the single source of truth. Clients never talk to each other
directly — every message, public or private, is routed **through** the
server, which is what lets it log everything, enforce unique usernames,
and broadcast consistently to every connected client.

## 2. Socket Flow

### Server side

```
socket()  -> creates an endpoint (AF_INET, SOCK_STREAM)
bind()    -> attaches the endpoint to 0.0.0.0:<port>
listen()  -> marks the socket as passive / ready to accept connections
accept()  -> blocks until a client connects, returns a NEW socket fd
             dedicated to that one client (the listening socket keeps
             listening for the next client)
recv()    -> reads bytes sent by that specific client
send()    -> writes bytes to that specific client
close()   -> releases the socket's OS resources
```

Every `accept()` call hands back a **distinct file descriptor** — this is
the detail that makes concurrent clients possible. The listening socket
is a factory for per-client sockets; it is never used to exchange chat
data itself.

### Client side

```
socket()    -> creates an endpoint
connect()   -> initiates the TCP three-way handshake with the server
send()/recv() -> exchange chat data over the now-established connection
close()     -> tears the connection down
```

### Message framing

TCP is a byte stream, not a message stream — it has no concept of
"where one message ends." NetConnect uses the simplest workable framing
scheme: every logical message ends in `\n`, and both sides read a chunk,
split on newlines, and treat each line as one message. This is sufficient
for a line-oriented chat protocol; a binary protocol would instead
length-prefix each frame.

## 3. Thread Model

NetConnect uses **thread-per-connection**, the standard first design for
a server with a moderate number of mostly-idle connections:

```
main thread
  └── acceptLoop()                     [blocks on accept()]
        ├── accept client 1 -> spawn std::thread -> handleClient(client1)
        ├── accept client 2 -> spawn std::thread -> handleClient(client2)
        └── accept client 3 -> spawn std::thread -> handleClient(client3)
```

Each `handleClient` thread is independently blocked on `recv()` for its
own socket, so one slow or silent client can never stall the others —
the OS scheduler and TCP stack handle the interleaving. Threads are
**detached** rather than joined individually, because their natural
lifetime is the client's connection; `Server::stop()` closes every
client socket, which unblocks each thread's `recv()` call with a
zero-byte read and lets it exit its loop and terminate on its own.

### Shared state and synchronization

The only state shared across threads is the server's user table
(`std::unordered_map<std::string, UserPtr>`), guarded by a single
`std::mutex` (`usersMutex_`). Every read or write of that map —
registering a user, looking one up for `/msg`, iterating it for
`broadcast()` — takes the lock for the shortest scope possible.

Each individual `User`'s socket also has its own `std::mutex` inside
`User::send()`, since two different threads (the client's own handler
thread, and another client's handler thread calling `broadcast()`) can
legitimately try to write to the same socket concurrently; without that
lock, interleaved `send()` calls could corrupt the byte stream a client
receives.

### Client-side threading

The client uses two threads for the opposite reason a server does:
not to serve many peers, but so one side (typing at a keyboard) and the
other (receiving broadcasts) don't block each other. The main thread
blocks on `std::getline(std::cin, ...)`; a second thread blocks on
`recv()` and prints whatever arrives. Without this split, a message
sent by another user wouldn't appear on screen until *after* you pressed
Enter on your own client.

## 4. Class Diagram (informal)

```
┌────────────────┐        ┌────────────────┐
│     Socket      │◄──────┤      User        │
│ (RAII fd owner)  │  has   │ (username + fd)  │
└────────────────┘        └───────┬────────┘
                                    │ managed by
                                    ▼
┌────────────────┐        ┌────────────────┐
│     Config       │──────►│      Server       │
│ (port, limits)    │  uses  │ (accept loop,      │
└────────────────┘        │  broadcast, cmds)  │
                            └───────┬────────┘
                                    │ uses
                                    ▼
                            ┌────────────────┐
                            │     Logger        │
                            │ (timestamped log)  │
                            └────────────────┘

┌────────────────┐        ┌────────────────┐
│     Socket       │◄──────┤      Client        │
└────────────────┘  has   │ (connect, send/recv,│
                            │  two threads)       │
                            └───────┬────────┘
                                    │ uses
                                    ▼
                            ┌────────────────┐
                            │     Logger        │
                            └────────────────┘
```

`Socket` is intentionally the only class that touches raw POSIX socket
calls. `Server`, `Client`, `User`, `Config`, and `Logger` never call
`::send`/`::recv`/`::bind` etc. directly — they go through `Socket`,
which keeps the OS-specific, error-prone code isolated in one place and
makes everything above it easier to reason about and test.

## 5. Why thread-per-connection instead of an event loop?

An event-driven design (`epoll`/`select`/`kqueue` with a single thread
multiplexing many sockets) scales to far more concurrent connections
with less memory overhead, since it avoids one OS thread per client.
NetConnect uses thread-per-connection instead because:

- It maps directly onto blocking `recv()`/`send()`, which keeps each
  client's protocol logic linear and easy to follow — no state machine
  needed to resume mid-message.
- A chat server's real bottleneck is rarely the number of *idle*
  connections; it's usually fine up to hundreds or low thousands of
  clients, which comfortably covers this project's scope.
- It's the natural teaching example for `std::thread`, `std::mutex`,
  and RAII — which was the point of building this project.

The trade-off is explicit and worth being able to name in an interview:
thread-per-connection trades memory/scalability ceiling for simplicity.
An `epoll`-based reactor would be the natural "what would you do
differently at scale" follow-up.
