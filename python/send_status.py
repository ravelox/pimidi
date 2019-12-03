#!/usr/bin/python

import socket
import struct
import sys

local_port = 5006

# Request status
bytes = struct.pack( "4s", "STAT" )
print bytes

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
s.setblocking(0)
s.connect( connect_tuple )
s.sendall( bytes )

data = ''
while True:
	try:
		data,addr = s.recvfrom(2)
	except:
		pass
	if data:
		break

print repr(data)
s.close()
