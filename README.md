# IRC Server (ft_irc)

A fully functional **IRC (Internet Relay Chat) server** implemented in **C++98**, developed as part of the 42 Network curriculum.  
This project demonstrates core concepts of **network programming**, **socket management**, and **multi-client communication** using the `poll()` system call.


## 📘 Project Overview

This project reimplements the behavior of a real IRC server following the **RFC 1459** protocol.  
It allows multiple clients to connect simultaneously, create and join channels, send private messages, and manage channel moderation through various IRC commands.


## 🧩 Features

- **Multi-client support** using non-blocking sockets and the `poll()` system call  
- **User authentication** with `PASS`, `NICK`, and `USER` commands  
- **Channel management** (`JOIN`, `PART`, `TOPIC`, `NAMES`, etc.)  
- **Private and channel messaging** via `PRIVMSG` and `NOTICE`  
- **Operator privileges** (`MODE`, `KICK`, `INVITE`, `TOPIC` commands)  
- **Graceful disconnection** and error handling  
- Compliant with **RFC 1459** specification  


## ⚙️ Project Structure

```
ircserv/
├── Makefile
├── main.cpp
├── Server.cpp / Server.hpp
├── Client.cpp / Client.hpp
├── Channel.cpp / Channel.hpp
├── Commands.cpp / Commands.hpp
└── .vscode/ (optional IDE configuration)
```

## 🏗️ Compilation

To build the project, simply run:

make


## 🚀 Usage

You can start the server by running:

./ircserv <port> <password>


Example:

./ircserv 6667 mypassword


<port>: The port number where the server will listen (e.g., 6667)

<password>: The connection password required by clients


## 💬 Connecting to the Server

You can connect using any IRC client such as:

KVirc

WeeChat

HexChat

Example connection:
/server localhost 6667 -password=mypassword


## 🧠 Implemented Commands

| Command   | Description                       |
| --------- | --------------------------------- |
| `PASS`    | Set a connection password         |
| `NICK`    | Set the user's nickname           |
| `USER`    | Register the user                 |
| `JOIN`    | Join or create a channel          |
| `PART`    | Leave a channel                   |
| `PRIVMSG` | Send a private or channel message |
| `NOTICE`  | Send a notice message             |
| `KICK`    | Remove a user from a channel      |
| `INVITE`  | Invite a user to a channel        |
| `TOPIC`   | Set or view the channel topic     |
| `MODE`    | Change channel/user modes         |
| `QUIT`    | Disconnect from the server        |


## 🧱 Code Highlights

Server class: Handles socket creation, connection management, and poll events.

Client class: Manages individual client states, nicknames, and message buffers.

Channel class: Stores channel members, topics, and operator privileges.

Commands module: Parses and executes all IRC protocol commands.

## 🧪 Example Interaction

[Server] Listening on port 6667
[Server] New client connected: 4
[Server] Client authenticated successfully.
[Server] Client joined channel: #general
[Server] <alperen> Hello everyone!


## 🧰 Requirements

C++98 compliant compiler (e.g., g++)
Unix-like OS (Linux, macOS)
Make build tool
