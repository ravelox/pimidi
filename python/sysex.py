#!/usr/bin/python

import sys
import socket
import struct
import time

local_port = 5006

if len(sys.argv) == 1:
        family = socket.AF_INET
        connect_tuple = ( 'localhost', local_port )
else:
        details = socket.getaddrinfo( sys.argv[1], local_port, socket.AF_UNSPEC, socket.SOCK_DGRAM)
        family = details[0][0]
        if family == socket.AF_INET6:
                connect_tuple = ( sys.argv[1], local_port, 0, 0)
        else:   
                connect_tuple = ( sys.argv[1], local_port)

s = socket.socket( family, socket.SOCK_DGRAM )
s.connect( connect_tuple )

# Complete SysEx
bytes = struct.pack( "BBBBBB", 0xf0, 0x1, 0x2, 0x3, 0x04, 0xf7 )
s.send( bytes )


# Partial SysEx start
bytes = struct.pack( "BBBBBB", 0xf0, 0x1, 0x2, 0x3, 0x04, 0xf0 )
s.send( bytes )

# Partial SysEx middle
bytes = struct.pack( "BBBBBB", 0xf7, 0x1, 0x2, 0x3, 0x04, 0xf0 )
s.send( bytes )

# Partial SysEx middle
bytes = struct.pack( "BBBBBB", 0xf7, 0x1, 0x2, 0x3, 0x04, 0xf0 )
s.send( bytes )

# Partial SysEx end
bytes = struct.pack( "BBBBBB", 0xf7, 0x1, 0x2, 0x3, 0x04, 0xf7 )
s.send( bytes )


# Complete SysEx with interleaved TimingClock
bytes = struct.pack( "BBBBBBB", 0xf0, 0x1, 0x2, 0xf8, 0x3, 0x04, 0xf7 )
s.send( bytes )


s.close()
