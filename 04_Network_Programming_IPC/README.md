# Task 4 — Network Programming and IPC

This task implements a multi-client TCP client-server application in C.

The server listens on `127.0.0.1:8080` and creates a separate POSIX thread for each connected client. Clients must authenticate before using protected commands.

## Source Files

- `server.c` — Multi-threaded TCP server
- `client.c` — Interactive TCP client

## Features

- Separate server and client applications
- TCP communication using IPv4 sockets
- Concurrent client handling using POSIX threads
- Username and password authentication
- Command-based communication protocol
- Empty and oversized input validation
- Safe handling of invalid credentials and unknown commands
- Unexpected client-disconnection handling
- Active-client counting protected by a mutex
- Timestamped server logging
- Graceful server shutdown using `Ctrl+C`

## Demonstration Accounts

| Username | Password |
|---|---|
| `saurav` | `student123` |
| `ankit` | `member123` |
| `guest` | `guest123` |

These credentials are hard-coded for coursework demonstration only and are not suitable for a production system.

## Compilation

Compile the server:

    gcc -Wall -Wextra -std=c11 server.c -o server -pthread

Compile the client:

    gcc -Wall -Wextra -std=c11 client.c -o client

## Execution

Start the server first:

    ./server

The server listens on:

    127.0.0.1:8080

Open another terminal and start the client:

    ./client

Additional clients can be started in separate terminals.

## Protocol Commands

| Command | Format | Purpose |
|---|---|---|
| `LOGIN` | `LOGIN <username> <password>` | Authenticate the client |
| `MSG` | `MSG <message>` | Send a message after authentication |
| `STATUS` | `STATUS` | Display the logged-in user and active-client count |
| `TIME` | `TIME` | Request the server timestamp |
| `HELP` | `HELP` | Display supported commands |
| `QUIT` | `QUIT` | Close the connection safely |

## Response Format

The server responses begin with:

- `OK` — Operation succeeded
- `INFO` — Informational response
- `ERROR` — Invalid or denied operation
- `BYE` — Connection is closing

Example responses:

    OK LOGIN successful
    ERROR Invalid credentials
    ERROR Authentication required
    INFO user=saurav authenticated=yes active_clients=2
    BYE Connection closed

## Logging

The server writes timestamped events to:

    server.log

Each record includes the client address, username, action and result. The log file is assigned Linux permission mode `0600`.

## Security Limitations

- TCP communication is not encrypted.
- Demonstration passwords are stored as plain text in the source code.
- Authentication does not use password hashing.
- Testing is performed through the local loopback interface.
- The server uses a fixed maximum message length.

Production improvements would include TLS, salted password hashes, secure credential storage and stronger rate limiting.

## Test Evidence

Test outputs are stored in the `outputs` directory and cover:

- Valid authentication and normal communication
- Invalid credentials
- Concurrent clients
- Malformed and unknown commands
- Empty and oversized input
- Unexpected disconnection
- Port conflict and graceful shutdown
