#!/usr/bin/env python3

import socket
import struct
import sys
import time

LOCAL_PORT = 5006

def note_send(sock):
    # Note ON
    msg = struct.pack("BBB", 0x96, 0x3C, 0x7F)
    sock.send(msg)

    time.sleep(0.25)

    # Note OFF
    msg = struct.pack("BBB", 0x86, 0x3C, 0x7F)
    sock.send(msg)
    print("note_send")

def control_send(sock):
    # Control Change
    msg = struct.pack("BBB", 0xB6, 0x3C, 0x7F)
    sock.send(msg)
    print("control_send")

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
        sock.connect(connect_tuple)

        print("Start")
        for i in range(100):
            print(i)
            note_send(sock)
            control_send(sock)
        print("End")


if __name__ == "__main__":
    main()
