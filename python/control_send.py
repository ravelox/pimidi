#!/usr/bin/env python3

import socket
import struct
import sys
import time

LOCAL_PORT = 5006

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

        # Control Change
        msg = struct.pack("BBB", 0xB6, 0x3C, 0x7F)
        sock.send(msg)
        msg = struct.pack("BBB", 0xB6, 0x3E, 0x7F)
        sock.send(msg)


if __name__ == "__main__":
    main()
