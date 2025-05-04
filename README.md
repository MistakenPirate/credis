# Credis - A Simple Redis-like Server in C

Credis is a minimalistic Redis-like server implemented in C. It supports basic key-value storage operations such as `SET`, `GET`, `DEL`, `EXISTS` and `PING`. The server uses multiplexing with `select()` to handle multiple client connections concurrently.

## Features

- **SET key value**: Stores a key-value pair in memory.
- **GET key**: Retrieves the value associated with a key.
- **DEL key**: Deletes the key-value pair in memory.
- **EXISTS key**: Checks is the value for a key exists.
- **PING**: Responds with `PONG` to check server availability.
- **Multiplexing**: Handles multiple client connections using `select()`.

## How to Build and Run

### Prerequisites

- A C compiler (e.g., `gcc`)
- Basic knowledge of terminal commands

### Steps

1. **Clone the Repository**:
   ```bash
   git clone https://github.com/yourusername/credis.git
   cd credis
   ```

2. **Compile the Code**:
   ```bash
   gcc -o credis main.c
   ```

3. **Run the Server**:
   ```bash
   ./credis
   ```
   By default, the server runs on `127.0.0.1:6379`.

   You can specify a custom host and port using the `-h` and `-p` options:
   ```bash
   ./credis -h 0.0.0.0 -p 8080
   ```

4. **Connect to the Server**:
   Use a Redis client (e.g., `redis-cli`) or `telnet` to connect to the server:
   ```bash
   redis-cli -h 127.0.0.1 -p 6379
   ```
   Or:
   ```bash
   telnet 127.0.0.1 6379
   ```

5. **Send Commands**:
   - Set a key-value pair:
     ```
     SET mykey myvalue
     ```
   - Get the value of a key:
     ```
     GET mykey
     ```
   - Ping the server:
     ```
     PING
     ```

## Code Structure

- **main.c**: Contains the main server logic, including command-line argument parsing, server initialization, and the event loop.
- **handle.h**: Defines the `KeyValue` struct and function prototypes for handling client requests.
- **multiplexing.c**: Implements the `handle()` function to process client commands and manage the key-value store.

## Limitations

- The server supports a maximum of 100 key-value pairs.
- It can handle up to 10 concurrent client connections.
- No persistence: All data is stored in memory and lost when the server stops.
- Basic error handling is implemented, but the server is not production-ready.

## Contributing

Contributions are welcome! If you'd like to improve Credis, feel free to open an issue or submit a pull request.

---

Enjoy experimenting with Credis! If you have any questions or feedback, feel free to reach out.
