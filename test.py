import socket

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(("127.0.0.1", 1234))
sock.sendall(b"PING\r\n")
data = sock.recv(1024)
print("Response:", data)
sock.close()
