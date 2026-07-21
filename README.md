# HyperCache (C++ Redis Implementation)

HyperCache is a high-throughput, Redis-compatible in-memory datastore built entirely from scratch in C++. It replaces standard thread-per-connection scaling with an event-driven, non-blocking I/O multiplexing architecture, demonstrating deep systems-level engineering and optimization.

## Key Performance Highlights
- **High Throughput**: Capable of processing over 53,000+ Requests Per Second (RPS) entirely on a single thread.
- **Non-Blocking I/O**: Utilizes `epoll` (Linux) / `poll` (Windows) for event-driven networking, allowing the server to handle thousands of concurrent connections without context-switching overhead.
- **Memory Efficiency**: Custom memory structures built specifically to minimize pointer overhead and memory fragmentation.

## Core Architecture & Systems Design

### 1. The Event Loop (I/O Multiplexing)
At the heart of the system is a single-threaded event loop. Rather than spawning a thread per client (which exhausts OS resources), it uses system multiplexing (`epoll_wait` / `sock_poll`) to monitor thousands of file descriptors simultaneously. Client states are buffered non-blockingly in custom `Conn` structures, ensuring the main thread is never stalled by slow network I/O.

### 2. Custom Data Structures
- **Top-Level Database (`HMap`)**: The core keyspace is an intrusive, chained hash table.
- **Progressive Resizing**: When load factors exceed thresholds, the hash table resizes progressively across multiple event loop cycles. This prevents the "stop-the-world" latency spikes common in naive implementations.
- **Sorted Sets (`ZSET`)**: Implemented using a dual-structure approach—a Hashtable for $O(1)$ point lookups combined with a custom AVL Tree for $O(\log N)$ range queries and rank operations.

### 3. Background Thread Pool
While the core engine is single-threaded to prevent locking overhead, heavy operations are safely offloaded. A custom 4-worker `ThreadPool` handles asynchronous tasks like the `UNLINK` command, which destroys massive data structures in the background without stalling the main event loop.

### 4. Protocol Parsing (RESP)
Includes a highly optimized parser for the Redis Serialization Protocol (RESP). It translates raw socket byte streams directly into internal command vectors and handles pipelined requests seamlessly by continuing to parse as long as the OS buffer has data.

## Storage & Persistence Mechanics

### AOF (Append-Only File)
Data durability is achieved via an AOF mechanism. It deliberately exploits the **OS Page Cache** by utilizing fast `fwrite()` calls without immediate `fflush()` blocking. This delegates the slow physical disk synchronization to the Operating System, achieving a perfect balance of data safety and extreme throughput.

### RDB Snapshots & Copy-on-Write (CoW)
On UNIX systems, the `BGSAVE` command clones the main process using the `fork()` syscall. It leverages the Linux kernel's **Copy-on-Write** (CoW) memory management to take a perfect point-in-time snapshot of the database (even Gigabytes in size) without pausing the main thread or wastefully duplicating physical RAM. 

## Expiration & Eviction (Memory Management)
- **Active Expiration**: Uses a Min-Heap (`std::priority_queue`) running inside the main event loop to actively purge expired keys based on TTL without requiring O(N) sweeps.
- **Passive Expiration**: Keys are lazily checked and deleted upon client access.
- **Approximated LRU**: To enforce memory caps, it implements a probabilistic eviction strategy. It randomly samples 5 keys and evicts the one with the oldest access timestamp, accurately simulating LRU eviction without the massive memory overhead of maintaining a global doubly-linked list.

## Supported Features
*   **Core Commands**: `GET`, `SET`, `DEL`, `INCR`, `KEYS`
*   **Time-To-Live (TTL)**: `EXPIRE`, `TTL`
*   **Sorted Sets**: `ZADD`, `ZREM`, `ZSCORE`, `ZQUERY`
*   **Pub/Sub**: `SUBSCRIBE`, `UNSUBSCRIBE`, `PUBLISH`
*   **Transactions**: `MULTI`, `EXEC`, `DISCARD`
*   **Persistence**: `SAVE`, `BGSAVE`
*   **Advanced Operations**: `SCAN` (cursor-based iteration without blocking)
