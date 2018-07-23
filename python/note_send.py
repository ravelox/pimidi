#!/usr/bin/python

import socket
import struct
import time

s = socket.socket( socket.AF_INET, socket.SOCK_DGRAM )

s.connect( ("localhost", 5006 ) )

# Note ON
bytes = struct.pack( "BBBB", 0xaa, 0x96, 0x3c, 0x7f )
s.send( bytes )

time.sleep( 0.25 );

# Note OFF
bytes = struct.pack( "BBBB", 0xaa, 0x86, 0x3c, 0x7f )
s.send( bytes )

s.close()
