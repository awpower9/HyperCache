import socket
import time

def send_recv(s, req):
    s.sendall(req)
    res = s.recv(1024)
    return res

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(("127.0.0.1", 1234))

print("--- Testing INCR ---")
print("INCR mycounter ->", send_recv(s, b"*2\r\n$4\r\nINCR\r\n$9\r\nmycounter\r\n"))
print("INCR mycounter ->", send_recv(s, b"*2\r\n$4\r\nINCR\r\n$9\r\nmycounter\r\n"))

print("\n--- Testing Transactions (MULTI/EXEC) ---")
print("MULTI ->", send_recv(s, b"*1\r\n$5\r\nMULTI\r\n"))
print("SET txkey ->", send_recv(s, b"*3\r\n$3\r\nSET\r\n$5\r\ntxkey\r\n$6\r\ntxval1\r\n"))
print("INCR txcounter ->", send_recv(s, b"*2\r\n$4\r\nINCR\r\n$9\r\ntxcounter\r\n"))
print("EXEC ->", send_recv(s, b"*1\r\n$4\r\nEXEC\r\n"))

print("\n--- Testing INFO Metrics ---")
print("INFO ->\n", send_recv(s, b"*1\r\n$4\r\nINFO\r\n").decode())

s.close()
