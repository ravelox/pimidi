#!/usr/bin/python

import socket
import struct

s = socket.socket( socket.AF_INET, socket.SOCK_DGRAM )

s.connect( ("localhost", 5006 ) )

# command/channel note velocity
bytes = struct.pack( "BBBB", 0xaa, 0x96, 0x3c, 0x7f )
s.send( bytes )

s.close()
