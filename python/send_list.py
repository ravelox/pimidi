#!/usr/bin/env python3

import socket
import struct
import sys

COMMAND_PORT = 5007  # network.command.port (was network.local.port / 5006 prior to the command-port split)

# Request status
SEND_BYTES = b"LIST"

def main():
    if len(sys.argv) == 1:
        family = socket.AF_INET
        connect_tuple = ("localhost", COMMAND_PORT)
    else:
        details = socket.getaddrinfo(
            sys.argv[1], COMMAND_PORT, socket.AF_UNSPEC, socket.SOCK_DGRAM
        )
        family = details[0][0]
        if family == socket.AF_INET6:
            connect_tuple = (sys.argv[1], COMMAND_PORT, 0, 0)
        else:
            connect_tuple = (sys.argv[1], COMMAND_PORT)

    with socket.socket(family, socket.SOCK_DGRAM) as sock:
        sock.settimeout(2.0)
        sock.connect(connect_tuple)
        sock.sendall(SEND_BYTES)

        try:
            data, _ = sock.recvfrom(8192)
        except socket.timeout:
            sys.stderr.write("Timed out waiting for raveloxmidi list response\n")
            sys.exit(1)

    print(data.decode(errors="ignore"))


if __name__ == "__main__":
    main()
