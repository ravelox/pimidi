#!/usr/bin/python

import socket
import select
import datetime
from dpkt import applemidi

class MIDIConnection:
	def __init__( self):
		self.ssrc = 0
		self.start_time = datetime.datetime.now()
	def get_ssrc(self):
		return self.ssrc
	def get_start_time(self):
		return self.start_time

connections = []

def find_connection( ssrc ):
	for c in connections:
		if c.ssrc == ssrc:
			return c
	return None

def timedelta_to_microtime(td):
	return td.microseconds + (td.seconds + td.days * 86400) * 1000000

#
# Build the list of sockets to listen on
#
HOST='0.0.0.0'
port5004 = socket.socket( socket.AF_INET , socket.SOCK_DGRAM )
port5005 = socket.socket( socket.AF_INET , socket.SOCK_DGRAM )

port5004.bind( (HOST, 5004) )
port5005.bind( (HOST, 5005) )

listen_sockets = [ port5004, port5005 ]


while True:
	read_socket,write_socket,error_socket = select.select(listen_sockets,[],[])

        for s in read_socket:
		buffer,address = s.recvfrom(1024)

		print "Connection from"
		print `address`

		packet = applemidi.COMMAND(buffer)
		print packet.command

		if packet.command == "IN":
			inv = applemidi.INVITATION( packet.data )
			print `inv`
			c = MIDIConnection( )
			c.ssrc = inv.ssrc
			connections.append( c )
			send_packet_inv = applemidi.INVITATION( version=2, initiator = inv.initiator, ssrc=0xDEADBEEF, data='acer.local\x00')
			print `send_packet_inv`
			send_packet = applemidi.COMMAND( signature = packet.signature, command='OK', data=send_packet_inv.pack())
			print  `send_packet`
			s.sendto( send_packet.pack() , address )

		if packet.command == "CK":
			sync = applemidi.SYNCHRONIZATION( packet.data )
			print `sync` 
			print "Count = " + str ( sync.count )
			c = find_connection( sync.ssrc )
			if c != None:
				if sync.count == 1:
					delta = timedelta_to_microtime( datetime.datetime.now() - c.get_start_time() )
				else:
					delta = sync.timestamp2

				if sync.count == 2:
					send_packet_count = 0
				else:
					send_packet_count = sync.count + 1
				send_packet_sync = applemidi.SYNCHRONIZATION( count=send_packet_count, padding="\x00\x00\x00", timestamp1=sync.timestamp1 , timestamp2=delta, timestamp3=sync.timestamp3, ssrc=0xDEADBEEF )
				send_packet = applemidi.COMMAND( signature = packet.signature, command="CK", data=send_packet_sync.pack())
				print `send_packet_sync`
				s.sendto( send_packet.pack(), address )

		if packet.command == "BY":
			inv = applemidi.INVITATION( packet.data )
			print `inv`
			c = find_connection( inv.ssrc )
			if c != None:
				connections.remove( c )
			
