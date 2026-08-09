# Screenshots

Add screenshots to the `screenshots/` folder and reference them below.
Suggested captures:

## Server startup

Shows the server binding to its configured port and printing the
color-coded startup log line.

```
![Server startup](../screenshots/server-startup.png)
```

## Multiple clients connected

Terminal windows for two or more clients side by side, showing the
username handshake and welcome message.

```
![Multiple clients](../screenshots/multiple-clients.png)
```

## Public broadcast message

One client sends a message; the others receive it with a timestamp.

```
![Broadcast message](../screenshots/broadcast-message.png)
```

## Private messaging (`/msg`)

Sender's and recipient's terminals side by side showing a private
message that other clients do not see.

```
![Private message](../screenshots/private-message.png)
```

## `/users` command output

```
![Users list](../screenshots/users-list.png)
```

## Server log file (`logs/server.log`)

Shows timestamp, IP address, username, and message for each event.

```
![Server log](../screenshots/server-log.png)
```

## Graceful shutdown (Ctrl+C)

Server console after `SIGINT`, showing clients being notified and
disconnected cleanly.

```
![Graceful shutdown](../screenshots/graceful-shutdown.png)
```
