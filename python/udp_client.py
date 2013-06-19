#!/usr/bin/python

import socket
import struct

s = socket.socket( socket.AF_INET, socket.SOCK_DGRAM )

s.connect( ("localhost", 5006 ) )

bytes = struct.pack( "Bbbb", 0xaa, 0x00, 0x27, 0x7f )
s.send( bytes )

#bytes = struct.pack( "Bbbb", 0xaa, 0x05, 0x53, 0x74 )
#s.send( bytes )

s.close()
