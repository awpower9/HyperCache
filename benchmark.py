import socket
import time
import threading

def send_recv(s, req):
    s.sendall(req)
    res = s.recv(1024)
    return res

# 1. Test INFO
print("=== Testing INFO command ===")
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(("127.0.0.1", 1234))
print(send_recv(s, b"*1\r\n$4\r\nINFO\r\n").decode('utf-8'))
s.close()

# 2. Benchmark
print("=== Running Benchmark ===")
NUM_THREADS = 10
REQUESTS_PER_THREAD = 5000
total_requests = NUM_THREADS * REQUESTS_PER_THREAD

def benchmark_worker():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
    s.connect(("127.0.0.1", 1234))
    req = b"*3\r\n$3\r\nSET\r\n$3\r\nfoo\r\n$3\r\nbar\r\n"
    for _ in range(REQUESTS_PER_THREAD):
        s.sendall(req)
        s.recv(1024)
    s.close()

start_time = time.time()
threads = []
for _ in range(NUM_THREADS):
    t = threading.Thread(target=benchmark_worker)
    t.start()
    threads.append(t)

for t in threads:
    t.join()
    
end_time = time.time()
duration = end_time - start_time
rps = total_requests / duration

print(f"Total Requests: {total_requests}")
print(f"Duration: {duration:.2f} seconds")
print(f"Requests Per Second (RPS): {rps:.2f}")

# 3. Test INFO again to see stats
print("=== Testing INFO command after benchmark ===")
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(("127.0.0.1", 1234))
print(send_recv(s, b"*1\r\n$4\r\nINFO\r\n").decode('utf-8'))
s.close()
