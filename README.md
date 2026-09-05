# C++ Multi-Client TCP Chat

A simple command-line chat application written in C++ using Windows Winsock.

This project is primarily a learning project for understanding socket programming, TCP communication, multithreading, synchronization, and connection management.

> **Status:** Work in Progress / Paused
> The basic chat functionality works, but some cleanup, error handling, and reliability improvements are still planned.

## Current Features

* TCP client-server communication
* Multiple clients can connect simultaneously
* Each connected client has its own receive thread
* Clients choose a name when connecting
* Messages are broadcast to all other connected clients
* Line-delimited message protocol using `\n`
* Handles partial `send()` operations
* Tracks client connection state
* Detects graceful and unexpected disconnects through `recv()`
* Uses mutexes to protect shared client data
* Uses a condition variable to notify the main thread about disconnected clients
* Client `/quit` command
* Graceful client-side socket shutdown

## Project Structure

### `server.cpp`

The server:

* creates and listens on a TCP socket on port `54000`
* accepts incoming clients
* stores connected clients in a shared client list
* creates a receive thread for each client
* receives and reconstructs newline-delimited messages
* broadcasts messages to other connected clients
* tracks client connection state
* joins and removes disconnected clients

Each client is represented using a `ClientInfo` structure containing information such as:

* name
* IP address
* port
* socket
* address
* connection state
* client thread

### `client.cpp`

The client:

* connects to `127.0.0.1:54000`
* asks the user for a name
* creates separate sending and receiving threads
* sends newline-delimited messages
* receives messages from the server
* supports `/quit`
* shuts down the socket to unblock the receive thread before final cleanup

## Concepts Practiced

This project was built mainly to learn and practice:

* TCP sockets
* Winsock
* `send()` and `recv()`
* partial sends
* TCP message framing
* threads
* mutexes
* condition variables
* atomic variables
* shared-resource synchronization
* socket shutdown vs socket closing
* connection lifecycle management
* pointer ownership
* `std::unique_ptr`
* `std::vector`
* iterator-based cleanup

## Message Protocol

Messages are separated using a newline:

```text
message\n
```

The first message sent by a client is treated as the client's name.

After that, messages are broadcast in a format similar to:

```text
[Name] Hello
```

## Example

Server:

```text
Waiting for Connection
User Connected
User Connected
```

Client:

```text
Connected to the Server
Enter you Name: Sina
Write your Message:
Hello
```

Another client receives:

```text
[Sina] Hello
```

## Known Limitations / TODO

The project is not considered finished yet.

Planned improvements include:

* Properly handle clients discovered as disconnected during `send()`
* Notify the main cleanup system when a send failure occurs
* Avoid holding the client-list mutex during potentially blocking network operations
* Improve Winsock error reporting
* Improve server shutdown and thread cleanup
* Handle more edge cases around unexpected disconnects
* Add join/leave notifications
* Add a `/list` command
* Improve console output and logging
* Further review socket and thread ownership

## Why This Project Exists

The purpose of this project is not to build a production-ready chat application.

It was created as a hands-on introduction to lower-level network programming before moving on to more advanced networking projects.

The project helped explore problems that are normally hidden by higher-level networking libraries, including:

* TCP being a byte stream rather than a message protocol
* receiving partial or multiple messages in one `recv()`
* partial `send()` operations
* simultaneous client connections
* synchronization between threads
* safely handling disconnected clients
* determining which part of a program owns resource cleanup

## Platform

Currently developed for Windows using:

```cpp
#include <WinSock2.h>
```

The current implementation therefore depends on Winsock and is not cross-platform.

## Status

Development is currently paused while work continues on other networking projects.

The repository is intentionally kept in its current state to preserve the development and learning history.
