import socket
import time
import threading

running = True
total_reqs = 0

def worker():
    global total_reqs
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        sock.connect(("127.0.0.1", 1234))
    except:
        return
        
    reqs = 0
    cmd = b"PING\r\n"
    while running:
        try:
            sock.sendall(cmd)
            data = sock.recv(4096)
            if not data:
                break
            reqs += data.count(b"+PONG")
        except:
            break
            
    total_reqs += reqs
    sock.close()

threads = []
for _ in range(4):
    t = threading.Thread(target=worker)
    t.start()
    threads.append(t)

print("Running benchmark for 3 seconds...")
time.sleep(3)
running = False

for t in threads:
    t.join()

print(f"Total Requests: {total_reqs}")
print(f"Requests Per Second (RPS): {total_reqs / 3}")
