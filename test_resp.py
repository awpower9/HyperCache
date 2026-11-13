import socket
import time

def send_recv(s, req):
    s.sendall(req)
    res = s.recv(1024)
    return res

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(("127.0.0.1", 1234))

# Test 1: PING
print("PING ->", send_recv(s, b"*1\r\n$4\r\nPING\r\n"))

# Test 2: SET and GET
print("SET ->", send_recv(s, b"*3\r\n$3\r\nSET\r\n$3\r\nfoo\r\n$3\r\nbar\r\n"))
print("GET ->", send_recv(s, b"*2\r\n$3\r\nGET\r\n$3\r\nfoo\r\n"))

# Test 3: TTL and EXPIRE
print("TTL (before) ->", send_recv(s, b"*2\r\n$3\r\nTTL\r\n$3\r\nfoo\r\n"))
print("EXPIRE ->", send_recv(s, b"*3\r\n$6\r\nEXPIRE\r\n$3\r\nfoo\r\n$1\r\n2\r\n"))
print("TTL (after EXPIRE 2s) ->", send_recv(s, b"*2\r\n$3\r\nTTL\r\n$3\r\nfoo\r\n"))

print("Waiting 3 seconds...")
time.sleep(3)

print("GET (after expiry) ->", send_recv(s, b"*2\r\n$3\r\nGET\r\n$3\r\nfoo\r\n"))

s.close()
