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

# Note ON
bytes = struct.pack( "BBBB", 0xaa, 0x90, 0x30, 0x40 )
s.send( bytes )

time.sleep( 0.25 );

# Note OFF
bytes = struct.pack( "BBBB", 0xaa, 0x80, 0x30, 0x40 )
s.send( bytes )

s.close()
