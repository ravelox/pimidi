#!/usr/bin/python

import socket
import struct
import time

s = socket.socket( socket.AF_INET, socket.SOCK_DGRAM )

s.connect( ("localhost", 5006 ) )

# Control Change
bytes = struct.pack( "BBBB", 0xaa, 0xB6, 0x3c, 0x7f )
s.send( bytes )
bytes = struct.pack( "BBBB", 0xaa, 0xB6, 0x3e, 0x7f )
s.send( bytes )

s.close()
