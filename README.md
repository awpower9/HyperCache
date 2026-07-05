# HyperCache

HyperCache is a high-throughput, Redis-compatible in-memory datastore built entirely in C++ from scratch. 
It replaces standard thread-per-connection scaling with an event-driven, non-blocking I/O multiplexing architecture (epoll).

## Features
- **High Throughput**: Capable of processing over 53,000+ Requests Per Second (RPS) on a single thread.
- **Progressive Resizing Hash Tables**: O(1) lookups that resize transparently in the background without blocking the main event loop.
- **AVL Trees**: Efficient $O(\log N)$ range queries and Sorted Sets (`ZSET`).
- **AOF Persistence**: Append-Only File data durability.
- **Approximated LRU**: Probabilistic key eviction strategies to cap memory usage.
- **Atomic Transactions**: `MULTI`, `EXEC`, and `DISCARD` support for executing multiple commands atomically.
