# Credis

A tiny Redis clone written in C. Built this to learn how Redis works under the hood.

## What it does

Basic key-value operations over TCP:

- `SET key value` - store a value
- `GET key` - retrieve it
- `DEL key` - delete it
- `EXISTS key` - check if it exists
- `PING` - health check (returns `PONG`)

## Features

- **Hash table storage** - O(1) lookups using djb2 hashing with chaining
- **Fast I/O multiplexing** - uses epoll on Linux, kqueue on macOS
- **RESP protocol** - works with standard Redis clients
- **Updates in place** - SET on existing keys overwrites instead of duplicating
- **Cross-platform** - runs on Linux and macOS

## Building

```bash
gcc -o credis main.c domain/multiplexing.c
```

## Running

```bash
./credis                      # runs on 127.0.0.1:6379
./credis -h 0.0.0.0 -p 8080   # custom host/port
```

Connect with `redis-cli` or any Redis client:

```bash
redis-cli -p 6379
> SET hello world
OK
> GET hello
"world"
```

## Project structure

```
main.c              - entry point, arg parsing
domain/
  handle.h          - data structures (hash table)
  multiplexing.c    - event loop, command handling
```

## Limitations

- In-memory only (no persistence)
- Not production ready

## Contributing

PRs welcome.
