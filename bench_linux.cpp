#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

std::atomic<bool> g_running{true};
std::atomic<int64_t> g_total_reqs{0};

void benchmark_worker() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return;

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return;
    }

    std::string pipeline_cmd = "";
    for(int i=0; i<50; i++) {
        pipeline_cmd += "PING\r\n";
    }
    
    char buffer[4096];

    int64_t reqs = 0;
    while (g_running.load(std::memory_order_relaxed)) {
        if (send(sock, pipeline_cmd.c_str(), pipeline_cmd.size(), 0) <= 0) break;
        int received = recv(sock, buffer, sizeof(buffer), 0);
        if (received <= 0) break;
        // count the PONG responses
        for (int i = 0; i < received - 4; i++) {
            if (buffer[i] == '+' && buffer[i+1] == 'P' && buffer[i+2] == 'O' && buffer[i+3] == 'N' && buffer[i+4] == 'G') {
                reqs++;
            }
        }
    }

    g_total_reqs.fetch_add(reqs, std::memory_order_relaxed);
    close(sock);
}

int main() {
    std::cout << "Starting WSL epoll benchmark..." << std::endl;

    const int num_threads = 8;
    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(benchmark_worker);
    }

    std::this_thread::sleep_for(std::chrono::seconds(3));
    g_running.store(false, std::memory_order_relaxed);

    for (auto& t : threads) {
        t.join();
    }

    int64_t total = g_total_reqs.load();
    std::cout << "Total Requests in 3 seconds: " << total << std::endl;
    std::cout << "Requests Per Second (RPS): " << (total / 3) << std::endl;

    return 0;
}
