#!/usr/bin/python

import socket
import struct
import time

def note_send():
	# Note ON
	bytes = struct.pack( "BBBB", 0xaa, 0x96, 0x3c, 0x7f )
	s.send( bytes )
	
	time.sleep( 0.25 );
	
	# Note OFF
	bytes = struct.pack( "BBBB", 0xaa, 0x86, 0x3c, 0x7f )
	s.send( bytes )
	print "note_send"

def control_send():
	# Control Change
	bytes = struct.pack( "BBBB", 0xaa, 0xB6, 0x3c, 0x7f )
	s.send( bytes )
	print "control_send"

s = socket.socket( socket.AF_INET, socket.SOCK_DGRAM )
s.connect( ("localhost", 5006 ) )

print "Start"
for i in range(0,100):
	print i
	note_send()
	control_send()
print "End"

s.close()
