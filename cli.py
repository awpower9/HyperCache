import socket

def encode_resp(cmd_parts):
    res = f"*{len(cmd_parts)}\r\n"
    for part in cmd_parts:
        encoded = part.encode('utf-8')
        res += f"${len(encoded)}\r\n{part}\r\n"
    return res.encode('utf-8')

def main():
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect(("127.0.0.1", 1234))
        print("Connected to HyperCache!\n")
        print("--- Available Commands ---")
        print("  Basic:  SET <key> <val>, GET <key>, DEL <key>, KEYS")
        print("  TTL:    EXPIRE <key> <seconds>, TTL <key>")
        print("  ZSet:   ZADD <zset> <score> <name>, ZSCORE <zset> <name>")
        print("          ZREM <zset> <name>")
        print("          ZQUERY <zset> <score> <name> <offset> <limit>")
        print("  Server: SAVE (saves to disk)")
        print("--------------------------\n")
    except Exception as e:
        print(f"Could not connect: {e}")
        return

    while True:
        try:
            line = input("Redis> ")
            parts = line.strip().split()
            if not parts:
                continue
            if parts[0].lower() in ('exit', 'quit'):
                break
                
            s.sendall(encode_resp(parts))
            res = s.recv(4096)
            
            if not res:
                break
            print(res.decode('utf-8').strip('\r\n'))
        except EOFError:
            break
        except Exception as e:
            print(f"Error: {e}")
            break

if __name__ == '__main__':
    main()
