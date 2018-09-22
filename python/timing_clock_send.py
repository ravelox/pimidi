#!/usr/bin/python

import socket
import struct
import time

s = socket.socket( socket.AF_INET, socket.SOCK_DGRAM )

s.connect( ("localhost", 5006 ) )

# Timing Clock
bytes = struct.pack( "BB", 0xaa, 0xF8)
11111000
s.send( bytes )

s.close()
