import socket
import time

def parse_resp(sock):
    def read_until_crlf():
        line = b""
        while True:
            char = sock.recv(1)
            if not char:
                return line
            line += char
            if line.endswith(b"\r\n"):
                return line[:-2]
    line = read_until_crlf()
    if not line:
        return None
    if line.startswith(b"+"):
        return line[1:].decode()
    elif line.startswith(b"-"):
        return line[1:].decode()
    elif line.startswith(b":"):
        return int(line[1:])
    elif line.startswith(b"$"):
        length = int(line[1:])
        if length == -1:
            return None
        data = sock.recv(length + 2)
        return data[:-2].decode()
    elif line.startswith(b"*"):
        length = int(line[1:])
        return [parse_resp(sock) for _ in range(length)]
    return line

def send_cmd(sock, *args):
    cmd = f"*{len(args)}\r\n"
    for arg in args:
        arg = str(arg)
        cmd += f"${len(arg)}\r\n{arg}\r\n"
    sock.sendall(cmd.encode())
    return parse_resp(sock)

def main():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(("127.0.0.1", 1234))
    
    print("Flushing DB (if possible) or just adding keys...")
    for i in range(15):
        send_cmd(s, "SET", f"key{i}", f"val{i}")
        
    print("Testing SCAN...")
    cursor = 0
    all_keys = set()
    while True:
        res = send_cmd(s, "SCAN", cursor, "COUNT", 5)
        print(f"SCAN {cursor} -> {res}")
        cursor = int(res[0])
        for k in res[1]:
            all_keys.add(k)
        if cursor == 0:
            break
            
    print(f"Total keys retrieved: {len(all_keys)}")
    
if __name__ == "__main__":
    main()
