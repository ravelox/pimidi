#!/usr/bin/env python3

import socket
import struct
import sys

LOCAL_PORT = 5006

# Request status
SEND_BYTES = b"STAT"
print(SEND_BYTES)

def main():
    if len(sys.argv) == 1:
        family = socket.AF_INET
        connect_tuple = ("localhost", LOCAL_PORT)
    else:
        details = socket.getaddrinfo(
            sys.argv[1], LOCAL_PORT, socket.AF_UNSPEC, socket.SOCK_DGRAM
        )
        family = details[0][0]
        if family == socket.AF_INET6:
            connect_tuple = (sys.argv[1], LOCAL_PORT, 0, 0)
        else:
            connect_tuple = (sys.argv[1], LOCAL_PORT)

    with socket.socket(family, socket.SOCK_DGRAM) as sock:
        sock.setblocking(False)
        sock.connect(connect_tuple)
        sock.sendall(SEND_BYTES)

        data = b""
        while True:
            try:
                data, _ = sock.recvfrom(2)
            except Exception:
                pass
            if data:
                break

    print(data.decode(errors="ignore"))


if __name__ == "__main__":
    main()
