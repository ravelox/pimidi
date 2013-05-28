#!/usr/bin/python

import Socket
import struct

s = socket.socket( socket.AF_INET, socket.SOCK_DGRAM )

s.connect( ("localhost", 5006 ) )

bytes = struct.pack( "Bbbb", 0xaa, 0x01, 0x35, 0x47 )

s.send( bytes )
