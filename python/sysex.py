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
        sock.connect(connect_tuple)

        # Complete SysEx
        msg = struct.pack("BBBBBB", 0xF0, 0x01, 0x02, 0x03, 0x04, 0xF7)
        sock.send(msg)

        # Partial SysEx start
        msg = struct.pack("BBBBBB", 0xF0, 0x01, 0x02, 0x03, 0x04, 0xF0)
        sock.send(msg)

        # Partial SysEx middle
        msg = struct.pack("BBBBBB", 0xF7, 0x01, 0x02, 0x03, 0x04, 0xF0)
        sock.send(msg)

        # Partial SysEx middle
        msg = struct.pack("BBBBBB", 0xF7, 0x01, 0x02, 0x03, 0x04, 0xF0)
        sock.send(msg)

        # Partial SysEx end
        msg = struct.pack("BBBBBB", 0xF7, 0x01, 0x02, 0x03, 0x04, 0xF7)
        sock.send(msg)

        # Complete SysEx with interleaved TimingClock
        msg = struct.pack("BBBBBBB", 0xF0, 0x01, 0x02, 0xF8, 0x03, 0x04, 0xF7)
        sock.send(msg)


if __name__ == "__main__":
    main()
