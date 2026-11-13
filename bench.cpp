#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

std::atomic<bool> g_running{true};
std::atomic<int64_t> g_total_reqs{0};

void benchmark_worker() {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) return;

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        closesocket(sock);
        return;
    }

    const char* ping_cmd = "PING\r\n";
    char buffer[1024];

    int64_t reqs = 0;
    while (g_running.load(std::memory_order_relaxed)) {
        if (send(sock, ping_cmd, 6, 0) == SOCKET_ERROR) break;
        if (recv(sock, buffer, sizeof(buffer), 0) <= 0) break;
        reqs++;
    }

    g_total_reqs.fetch_add(reqs, std::memory_order_relaxed);
    closesocket(sock);
}

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);

    std::cout << "Starting benchmark..." << std::endl;

    const int num_threads = 4;
    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(benchmark_worker);
    }

    // Run for 3 seconds
    std::this_thread::sleep_for(std::chrono::seconds(3));
    g_running.store(false, std::memory_order_relaxed);

    for (auto& t : threads) {
        t.join();
    }

    int64_t total = g_total_reqs.load();
    std::cout << "Total Requests in 3 seconds: " << total << std::endl;
    std::cout << "Requests Per Second (RPS): " << (total / 3) << std::endl;

    WSACleanup();
    return 0;
}
