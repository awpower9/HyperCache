#pragma once

// ============================================================================
// Platform Detection & Includes
// ============================================================================

#ifdef _WIN32
    #define NOMINMAX
    #define WIN32_LEAN_AND_MEAN
    #ifndef _WIN32_WINNT
        #define _WIN32_WINNT 0x0600  // Windows Vista+ for WSAPoll
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")

    typedef SOCKET socket_t;
    #define SOCKET_INVALID INVALID_SOCKET

    // ssize_t is not defined in MSVC
    #ifdef _MSC_VER
        #include <BaseTsd.h>
        typedef SSIZE_T ssize_t;
    #endif
#else
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>
    #include <netinet/ip.h>
    #include <netinet/tcp.h>
    #include <fcntl.h>
    #include <poll.h>
    #include <sys/epoll.h>
    #include <errno.h>

    typedef int socket_t;
    #define SOCKET_INVALID (-1)
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

// ============================================================================
// Constants
// ============================================================================

const size_t k_max_msg = 4096;      // Max message payload size
const size_t k_max_args = 200;      // Max number of command arguments

// FNV hash function for strings
static inline uint64_t str_hash(const uint8_t *data, size_t len) {
    uint32_t h = 0x811C9DC5;                // FNV offset basis
    for (size_t i = 0; i < len; i++) {
        h = (h + data[i]) * 0x01000193;     // FNV prime
    }
    return (uint64_t)h;
}

// ============================================================================
// Serialization Type Tags (Chapter 8)
// ============================================================================

enum {
    SER_NIL = 0,    // Null value
    SER_ERR = 1,    // Error: code + message
    SER_STR = 2,    // String: length + data
    SER_INT = 3,    // 64-bit integer
    SER_ARR = 5,    // Array: count + elements
};

// ============================================================================
// Utility Functions
// ============================================================================

static inline void msg(const char *s) {
    fprintf(stderr, "%s\n", s);
}

static inline void die(const char *s) {
#ifdef _WIN32
    int err = WSAGetLastError();
#else
    int err = errno;
#endif
    fprintf(stderr, "[%d] %s\n", err, s);
    abort();
}

// ============================================================================
// container_of macro (intrusive data structure pattern)
// Given a pointer to a member, recover the pointer to the enclosing struct.
// ============================================================================

#define container_of(ptr, T, member) \
    ((T *)((char *)(ptr) - offsetof(T, member)))

// ============================================================================
// Platform-Specific Socket Operations
// ============================================================================

// Initialize the socket subsystem (required on Windows, no-op on Linux)
static inline void socket_init() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        die("WSAStartup failed");
    }
#endif
}

// Cleanup the socket subsystem
static inline void socket_cleanup() {
#ifdef _WIN32
    WSACleanup();
#endif
}

// Set a socket to non-blocking mode
static inline void fd_set_nb(socket_t fd) {
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(fd, FIONBIO, &mode);
#else
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#endif
}

// Close a socket
static inline void close_socket(socket_t fd) {
#ifdef _WIN32
    closesocket(fd);
#else
    close(fd);
#endif
}

// Read from a socket (non-blocking safe)
static inline ssize_t sock_read(socket_t fd, void *buf, size_t len) {
#ifdef _WIN32
    return recv(fd, (char *)buf, (int)len, 0);
#else
    return read(fd, buf, len);
#endif
}

// Write to a socket (non-blocking safe)
static inline ssize_t sock_write(socket_t fd, const void *buf, size_t len) {
#ifdef _WIN32
    return send(fd, (const char *)buf, (int)len, 0);
#else
    return write(fd, buf, len);
#endif
}

// Check if the last socket error was "would block" (EAGAIN/EWOULDBLOCK)
static inline bool sock_would_block() {
#ifdef _WIN32
    return WSAGetLastError() == WSAEWOULDBLOCK;
#else
    return errno == EAGAIN || errno == EWOULDBLOCK;
#endif
}

// Check if the last socket error was "interrupted" (EINTR)
static inline bool sock_interrupted() {
#ifdef _WIN32
    return WSAGetLastError() == WSAEINTR;
#else
    return errno == EINTR;
#endif
}

// Poll wrapper: uses WSAPoll on Windows, poll() on Linux
// struct pollfd is defined in winsock2.h (Windows) and <poll.h> (Linux)
static inline int sock_poll(struct pollfd *fds, size_t nfds, int timeout_ms) {
#ifdef _WIN32
    return WSAPoll(fds, (ULONG)nfds, timeout_ms);
#else
    return poll(fds, (nfds_t)nfds, timeout_ms);
#endif
}
