// ============================================================================
// client.cpp — Mini Redis Client
//
// A command-line client for the mini-Redis server.
// Uses blocking IO (simpler than the server — only one connection).
//
// Usage:
//   client set mykey myvalue
//   client get mykey
//   client del mykey
//   client keys
//
// Wire Protocol:
//   Request:  [4B len] [4B nstr] [4B str1_len] [str1] ...
//   Response: [4B len] [type-tagged payload]
//
// Compile (MSVC):  cl /EHsc /std:c++17 client.cpp /Fe:client.exe
// Compile (g++):   g++ -std=c++17 -o client client.cpp -lws2_32
// ============================================================================

#include "common.h"
#include <string>
#include <vector>

// ============================================================================
// Blocking IO Helpers
// The client uses blocking IO since it only handles one request at a time.
// ============================================================================

// Read exactly `n` bytes from the socket (blocking).
static int32_t read_full(socket_t fd, char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = sock_read(fd, buf, n);
        if (rv <= 0) {
            return -1;  // Error or unexpected EOF
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

// Write exactly `n` bytes to the socket (blocking).
static int32_t write_all(socket_t fd, const char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = sock_write(fd, buf, n);
        if (rv <= 0) {
            return -1;  // Error
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

// ============================================================================
// Request Encoding
// ============================================================================

// Send a command to the server.
// Format: [4B body_len] [4B nstr] [4B len1] [str1] [4B len2] [str2] ...
static int32_t send_req(socket_t fd, const std::vector<std::string> &cmd) {
    // Build the request body
    uint32_t nstr = (uint32_t)cmd.size();
    std::vector<uint8_t> body;

    // Number of strings
    body.insert(body.end(), (uint8_t *)&nstr, (uint8_t *)&nstr + 4);

    // Each string: [4B length] [string data]
    for (const std::string &s : cmd) {
        uint32_t len = (uint32_t)s.size();
        body.insert(body.end(), (uint8_t *)&len, (uint8_t *)&len + 4);
        body.insert(body.end(), s.begin(), s.end());
    }

    // Check size limit
    if (body.size() > k_max_msg) {
        msg("request too large");
        return -1;
    }

    // Send: [4B total_len] [body]
    char wbuf[4 + k_max_msg];
    uint32_t len = (uint32_t)body.size();
    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], body.data(), body.size());
    return write_all(fd, wbuf, 4 + body.size());
}

// ============================================================================
// Response Parsing & Display
// ============================================================================

// Forward declaration
static size_t print_response(const uint8_t *data, size_t len, size_t indent);

// Print one typed value from the response stream.
// Returns the number of bytes consumed.
static size_t print_response(const uint8_t *data, size_t len, size_t indent) {
    if (len < 1) {
        printf("(bad response)\n");
        return 0;
    }

    // Print indentation
    for (size_t i = 0; i < indent; i++) {
        printf("  ");
    }

    switch (data[0]) {
    case SER_NIL:
        printf("(nil)\n");
        return 1;

    case SER_ERR:
        if (len < 1 + 4 + 4) {
            printf("(bad err)\n");
            return 0;
        }
        {
            int32_t code = 0;
            memcpy(&code, &data[1], 4);
            uint32_t msg_len = 0;
            memcpy(&msg_len, &data[5], 4);
            if (1 + 4 + 4 + msg_len > len) {
                printf("(bad err: truncated)\n");
                return 0;
            }
            printf("(err) %d %.*s\n", code, (int)msg_len, &data[9]);
            return 1 + 4 + 4 + msg_len;
        }

    case SER_STR:
        if (len < 1 + 4) {
            printf("(bad str)\n");
            return 0;
        }
        {
            uint32_t str_len = 0;
            memcpy(&str_len, &data[1], 4);
            if (1 + 4 + str_len > len) {
                printf("(bad str: truncated)\n");
                return 0;
            }
            printf("\"%.*s\"\n", (int)str_len, &data[5]);
            return 1 + 4 + str_len;
        }

    case SER_INT:
        if (len < 1 + 8) {
            printf("(bad int)\n");
            return 0;
        }
        {
            int64_t val = 0;
            memcpy(&val, &data[1], 8);
            printf("(int) %lld\n", (long long)val);
            return 1 + 8;
        }

    case SER_ARR:
        if (len < 1 + 4) {
            printf("(bad arr)\n");
            return 0;
        }
        {
            uint32_t count = 0;
            memcpy(&count, &data[1], 4);
            printf("(arr) len=%u\n", count);

            size_t pos = 5;  // After type byte + count
            for (uint32_t i = 0; i < count; i++) {
                if (pos >= len) {
                    for (size_t j = 0; j < indent + 1; j++) printf("  ");
                    printf("(truncated)\n");
                    break;
                }
                size_t consumed = print_response(&data[pos], len - pos, indent + 1);
                if (consumed == 0) break;   // Parse error
                pos += consumed;
            }
            return pos;
        }

    default:
        printf("(unknown type %d)\n", data[0]);
        return 0;
    }
}

// Read one response from the server and print it.
static int32_t read_res(socket_t fd) {
    // Read the 4-byte length header
    char rbuf[4 + k_max_msg + 1];
    if (read_full(fd, rbuf, 4)) {
        msg("read() error (header)");
        return -1;
    }

    uint32_t len = 0;
    memcpy(&len, rbuf, 4);
    if (len > k_max_msg) {
        msg("response too long");
        return -1;
    }

    // Read the response body
    if (read_full(fd, &rbuf[4], len)) {
        msg("read() error (body)");
        return -1;
    }

    // Parse and display
    print_response((uint8_t *)&rbuf[4], len, 0);
    return 0;
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: client <command> [args...]\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "Commands:\n");
        fprintf(stderr, "  set <key> <value>   Set a key-value pair\n");
        fprintf(stderr, "  get <key>           Get the value for a key\n");
        fprintf(stderr, "  del <key>           Delete a key\n");
        fprintf(stderr, "  keys                List all keys\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "Examples:\n");
        fprintf(stderr, "  client set name Alice\n");
        fprintf(stderr, "  client get name\n");
        fprintf(stderr, "  client del name\n");
        fprintf(stderr, "  client keys\n");
        return 1;
    }

    // Initialize platform socket subsystem
    socket_init();

    // Create a TCP socket
    socket_t fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == SOCKET_INVALID) {
        die("socket()");
    }

    // Connect to the server at 127.0.0.1:1234
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (connect(fd, (const struct sockaddr *)&addr, sizeof(addr))) {
        die("connect() - is the server running?");
    }

    // Build command from argv
    std::vector<std::string> cmd;
    for (int i = 1; i < argc; i++) {
        cmd.push_back(argv[i]);
    }

    // Send the request
    int32_t err = send_req(fd, cmd);
    if (err) {
        goto done;
    }

    // Read and display the response
    err = read_res(fd);
    if (err) {
        goto done;
    }

done:
    close_socket(fd);
    socket_cleanup();
    return err ? 1 : 0;
}
