#!/usr/bin/python

import socket
import struct

# Request status
bytes = struct.pack( "B4s", 0xaa, "STAT" )
print bytes

s = socket.socket( socket.AF_INET, socket.SOCK_DGRAM )
s.connect( ("localhost", 5006 ) )
s.sendall( bytes )

data = ''
while True:
	data = s.recv(2)
	if data:
		break

print repr(data)
s.close()
