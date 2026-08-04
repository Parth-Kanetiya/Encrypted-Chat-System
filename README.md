# Encrypted Chat System

A multi-client chat system built in C++ using TCP sockets with XOR-based encryption. The system allows users to communicate securely through a central server.

## Features

- Multi-client chat using sockets
- Encrypted communication using XOR cipher
- Broadcast messaging
- Direct messaging (/msg)
- View online users (/list)
- Username validation (no duplicates)
- Server announcements (join/leave)
- Multi-threaded server handling

## Technologies & Concepts Used

- C++ (Standard Library)
- Socket Programming (TCP/IP)
- Multithreading (std::thread)
- Synchronization (mutex)
- XOR-based Encryption
- Custom Communication Protocol (TYPE|DATA format)
- Data Structures (maps for user management)

## Project Structure

- server.cpp       - Handles multiple clients and message routing
- client.cpp       - Client interface for sending/receiving messages
- crypto.cpp/h     - XOR encryption and key generation
- protocol.cpp/h   - Packet structure and parsing
- makefile         - Build automation

## How It Works

- Server generates a shared encryption key
- Client connects and receives the key
- All messages are encrypted using XOR cipher
- Messages are sent in a structured format: TYPE|DATA
- Server forwards messages to other clients

## Commands

- `/msg <user> <text>`  - Send private message
- `/list`               - Show online users
- `/quit`               - Exit chat

## Compilation

Using make:
```
make
```

Or manually:
```
g++ server.cpp protocol.cpp crypto.cpp -o server -pthread
g++ client.cpp protocol.cpp crypto.cpp -o client -pthread
```

## Usage

1. Start server:
```
./server
```

2. Start client (in another terminal):
```
./client
```

3. Enter username and start chatting

