/*
   This file is part of raveloxmidi.

   Copyright (C) 2014 Dave Kelly

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>

#include <pthread.h>

#include <errno.h>
extern int errno;

#include "config.h"

#include "net_applemidi.h"
#include "net_response.h"
#include "net_socket.h"
#include "net_connection.h"
#include "net_distribute.h"

#include "applemidi_inv.h"
#include "applemidi_ok.h"
#include "applemidi_sync.h"
#include "applemidi_feedback.h"
#include "applemidi_by.h"

#include "midi_note.h"
#include "midi_control.h"
#include "midi_program.h"

#include "rtp_packet.h"
#include "midi_command.h"
#include "midi_payload.h"
#include "utils.h"

#include "raveloxmidi_config.h"
#include "logging.h"

#include "raveloxmidi_alsa.h"

static int num_sockets = 0;
static int *sockets = NULL;
static int net_socket_shutdown;
static int inbound_midi_fd = -1;
static unsigned char *packet = NULL;
static size_t packet_size = 0;
#ifdef HAVE_ALSA
static size_t alsa_buffer_size = 0;
#endif

static fd_set read_fds;
static int max_fd = 0;

static pthread_mutex_t shutdown_lock;
static pthread_mutex_t socket_mutex;
pthread_t alsa_listener_thread;
int socket_timeout = 0;
int pipe_fd[2] = { 0, 0 };

static void set_shutdown_lock( int i );

void net_socket_lock( void )
{
	pthread_mutex_lock( &socket_mutex );
}

void net_socket_unlock( void )
{
	pthread_mutex_unlock( &socket_mutex );
}

void net_socket_add( int new_socket )
{
	int *new_socket_list = NULL;

	if( new_socket < 0 ) return;

	num_sockets++;
	new_socket_list = (int *) realloc( sockets, sizeof(int) * num_sockets );
	if( ! new_socket_list )
	{
		logging_printf(LOGGING_ERROR, "net_socket_add: Insufficient memory to create socket %d list entry\n", num_sockets );
	}
	sockets = new_socket_list;
	sockets[num_sockets - 1 ] = new_socket;
}

int net_socket_create(int family, char *ip_address, unsigned int port )
{
	int new_socket;

	logging_printf(LOGGING_DEBUG, "net_socket_create: Creating socket for [%s]:%u, family=%d\n", ip_address, port, family);

	new_socket = socket(family, SOCK_DGRAM, 0);
	if( new_socket < 0 )
	{
		return errno;
	}

	return new_socket;
}

int net_socket_listener_create( int family, char *ip_address, unsigned int port )
{
	int new_socket = -1;
	struct sockaddr_in6 socket_address;
	socklen_t addr_len = 0;
	int optionvalue = 0;
	
	new_socket = net_socket_create( family, ip_address, port );

	if( new_socket < 0 )
	{ 
		logging_printf( LOGGING_ERROR, "net_socket_listener_create: Unable to create socket %s:%u : %s\n", ip_address, port, strerror(new_socket));
		return new_socket;
	}

	switch( family )
	{
		case AF_INET6:
			optionvalue = 0;
			setsockopt( new_socket, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&optionvalue, sizeof( optionvalue ) );
			break;
		default:
			break;
	}
			
	get_sock_info( ip_address, port, (struct sockaddr *)&socket_address, &addr_len, NULL);

	if ( bind(new_socket, (struct sockaddr *)&socket_address, addr_len) < 0 )
	{       
		return errno;
        } 

	net_socket_add( new_socket );

	fcntl(new_socket, F_SETFL, O_NONBLOCK);

	return 0;
}

int net_socket_teardown( void )
{
	int socket;

	for(socket = 0 ; socket < num_sockets ; socket++ )
	{
#ifdef HAVE_ALSA
		if( sockets[socket] == RAVELOXMIDI_ALSA_INPUT ) continue;
#endif
		close( sockets[socket] );
	}

	free(sockets);

	if( inbound_midi_fd >= 0 ) close(inbound_midi_fd);
	if( packet ) FREENULL( "net_socket_teardown: packet", (void **)&packet );

	return 0;
}

int net_socket_read( int fd )
{
	int recv_len;
	unsigned from_len = 0;
	struct sockaddr_storage from_addr;
	int output_enabled = 0;
	char ip_address[ INET6_ADDRSTRLEN ];
	uint16_t from_port = 0;

	net_applemidi_command *command;
	int ret = 0;

	memset( ip_address, 0, INET6_ADDRSTRLEN );
	from_len = sizeof( from_addr );

	while( 1 )
	{
		memset( packet, 0, packet_size + 1 );
#ifdef HAVE_ALSA
		if( fd == RAVELOXMIDI_ALSA_INPUT )
		{
			recv_len = raveloxmidi_alsa_read( packet, alsa_buffer_size);
		} else {
#endif
			recv_len = recvfrom( fd, packet, NET_APPLEMIDI_UDPSIZE, 0, (struct sockaddr *)&from_addr, &from_len );
			get_ip_string( (struct sockaddr *)&from_addr, ip_address, INET6_ADDRSTRLEN );
			from_port = ntohs( ((struct sockaddr_in *)&from_addr)->sin_port );
#ifdef HAVE_ALSA
		}
#endif
		if ( recv_len <= 0)
		{   
			if ( errno == EAGAIN )
			{
				logging_printf( LOGGING_DEBUG, "EAGAIN\n");
				break;
			}
			logging_printf(LOGGING_ERROR, "net_socket_read: Socket error (%d) on socket (%d)\n", errno , fd );
			break;
		}

#ifdef HAVE_ALSA
		if( fd != RAVELOXMIDI_ALSA_INPUT )
		{
#endif
			logging_printf( LOGGING_DEBUG, "net_socket_read: read socket=%d, bytes=%u, host=%s, port=%u, first_byte=%02x)\n", fd, recv_len,ip_address, from_port, packet[0]);

#ifdef HAVE_ALSA
		} else {
			logging_printf( LOGGING_DEBUG, "net_socket_read: read socket=ALSA bytes=%u first_byte=%02x\n", recv_len, packet[0] );
		}
#endif
		
		hex_dump( packet, recv_len );

		// Apple MIDI command
		if( packet[0] == 0xff )
		{
			net_response_t *response = NULL;

			ret = net_applemidi_unpack( &command, packet, recv_len );
			net_applemidi_command_dump( command );

			switch( command->command )
			{
				case NET_APPLEMIDI_CMD_INV:
					response = applemidi_inv_responder( ip_address, from_port, command->data );
					break;
				case NET_APPLEMIDI_CMD_ACCEPT:
					response = applemidi_ok_responder( ip_address, from_port, command->data );
					break;
				case NET_APPLEMIDI_CMD_REJECT:
					logging_printf( LOGGING_ERROR, "net_socket_read: Connection rejected host=%s, port=%u\n", ip_address, from_port);
					break;
				case NET_APPLEMIDI_CMD_END:
					response = applemidi_by_responder( command->data );
					break;
				case NET_APPLEMIDI_CMD_SYNC:
					response = applemidi_sync_responder( command->data );
					break;
				case NET_APPLEMIDI_CMD_FEEDBACK:
					response = applemidi_feedback_responder( command->data );
					break;
				case NET_APPLEMIDI_CMD_BITRATE:
					break;
			}

			if( response )
			{
				size_t bytes_written = 0;
				net_socket_lock();
				bytes_written = sendto( fd, response->buffer, response->len , MSG_CONFIRM, (void *)&from_addr, from_len);
				net_socket_unlock();
				logging_printf( LOGGING_DEBUG, "net_socket_read: response write(bytes=%u,socket=%d,host=%s,port=%u)\n", bytes_written, fd,ip_address, from_port );	
				net_response_destroy( &response );
			}

			net_applemidi_cmd_destroy( &command );
		} else if( (packet[0]==0xaa) && (recv_len == 5) && ( strncmp( (const char *)&(packet[1]),"STAT",4)==0) )
		// Heartbeat request
		{
			char *buffer="OK";
			size_t bytes_written = 0;
			net_socket_lock();
			bytes_written = sendto( fd, buffer, strlen(buffer), MSG_CONFIRM, (void *)&from_addr, from_len);
			net_socket_unlock();
			logging_printf(LOGGING_DEBUG, "net_socket_read: Heartbeat request. Response written: %u\n", bytes_written);
		
		} else if( (packet[0]==0xaa) && (recv_len == 5) && ( strncmp( (const char *)&(packet[1]),"QUIT",4)==0) )
		// Shutdown request
		{
			char *buffer="QT";
			size_t bytes_written = 0;
			net_socket_lock();
			bytes_written = sendto( fd, buffer, strlen(buffer), MSG_CONFIRM, (void *)&from_addr, from_len);
			net_socket_unlock();
			logging_printf(LOGGING_DEBUG, "net_socket_read: Shutdown request. Response written: %u\n", bytes_written);
			logging_printf(LOGGING_NORMAL, "Shutdown request received on local socket\n");
			set_shutdown_lock(1);
#ifdef HAVE_ALSA
		} else if( (packet[0]==0xaa) || (fd==RAVELOXMIDI_ALSA_INPUT) )
#else
		} else if( packet[0] == 0xaa )
#endif
		// MIDI data on internal socket or ALSA rawmidi device
		{
			net_distribute_midi( packet, recv_len );
		} else {
		// RTP MIDI inbound from remote socket
			rtp_packet_t *rtp_packet = NULL;
			midi_payload_t *midi_payload=NULL;
			midi_command_t *midi_commands=NULL;
			size_t num_midi_commands=0;
			net_response_t *response = NULL;
			size_t midi_command_index = 0;

			rtp_packet = rtp_packet_create();
			rtp_packet_unpack( packet, recv_len, rtp_packet );
			logging_printf(LOGGING_DEBUG, "net_socket_read: inbound MIDI received\n");
			rtp_packet_dump( rtp_packet );

			midi_payload_unpack( &midi_payload, rtp_packet->payload, recv_len );

			// Read all the commands in the packet into an array
			midi_payload_to_commands( midi_payload, MIDI_PAYLOAD_RTP, &midi_commands, &num_midi_commands );

			// Sent a FEEBACK packet back to the originating host to ack the MIDI packet
			response = applemidi_feedback_create( rtp_packet->header.ssrc, rtp_packet->header.seq );
			if( response )
			{
				size_t bytes_written = 0;
				net_socket_lock();
				bytes_written = sendto( fd, response->buffer, response->len , MSG_CONFIRM, (void *)&from_addr, from_len);
				net_socket_unlock();
				logging_printf( LOGGING_DEBUG, "net_socket_read: feedback write(bytes=%u,socket=%d,host=%s,port=%u)\n", bytes_written, fd,ip_address, from_port);
				net_response_destroy( &response );
			}

			// Determine if the MIDI commands need to be written out
			output_enabled = ( inbound_midi_fd >= 0 );
#ifdef HAVE_ALSA
			output_enabled |= raveloxmidi_alsa_out_available();
#endif
			if( output_enabled )
			{
				logging_printf(LOGGING_DEBUG, "net_socket_read: output_enabled\n");
				for( midi_command_index = 0 ; midi_command_index < num_midi_commands ; midi_command_index++ )
				{
					unsigned char *raw_buffer = (unsigned char *)malloc( 2 + midi_commands[midi_command_index].data_len );

					if( raw_buffer )
					{
						size_t bytes_written = 0;
						raw_buffer[0]=midi_commands[midi_command_index].status;
						if( midi_commands[midi_command_index].data_len > 0 )
						{
							memcpy( raw_buffer + 1, midi_commands[midi_command_index].data, midi_commands[midi_command_index].data_len );
						}

						if( inbound_midi_fd >= 0 )
						{
							net_socket_lock();
							bytes_written = write( inbound_midi_fd, raw_buffer, 1 + midi_commands[midi_command_index].data_len );
							net_socket_unlock();
							logging_printf( LOGGING_DEBUG, "net_socket_read: inbound MIDI write(bytes=%u)\n", bytes_written );
						}

#ifdef HAVE_ALSA
						net_socket_lock();
						raveloxmidi_alsa_write( raw_buffer, 1 + midi_commands[midi_command_index].data_len );
						net_socket_unlock();
#endif
						free( raw_buffer );
					}
				}
			}

			// Clean up
			midi_payload_destroy( &midi_payload );
			for( ; num_midi_commands >= 1 ; num_midi_commands-- )
			{
				midi_command_reset( &(midi_commands[num_midi_commands - 1]) );
			}
			free( midi_commands );
			rtp_packet_destroy( &rtp_packet );
		}
	}
	return ret;
}

static void set_shutdown_lock( int i )
{
	pthread_mutex_lock( &shutdown_lock );
	net_socket_shutdown = i;
	pthread_mutex_unlock( &shutdown_lock );
}

int net_socket_get_shutdown_lock( void )
{
	int i = 0;
	pthread_mutex_lock( &shutdown_lock );
	i = net_socket_shutdown;
	pthread_mutex_unlock( &shutdown_lock );
	return i;
}

void net_socket_loop_init()
{
	int err = 0;

	pthread_mutex_init( &shutdown_lock, NULL);
	pthread_mutex_init( &socket_mutex, NULL );
	set_shutdown_lock(0);
	socket_timeout = config_long_get("network.socket_timeout");

// Set up a pipe as a shutdown signal
	err=pipe( pipe_fd );
	if( err < 0 )
	{
		logging_printf( LOGGING_ERROR, "net_socket_loop_init: pipe error: %s\n", strerror(errno));
	} else {
		logging_printf(LOGGING_DEBUG, "net_socket_loop_init: pipe0=%d pipe1=%d\n", pipe_fd[0], pipe_fd[1]);
		FD_SET( pipe_fd[1], &read_fds );
	}
}

void net_socket_loop_teardown()
{
	pthread_mutex_destroy( &shutdown_lock );
	pthread_mutex_destroy( &socket_mutex );
}

int net_socket_fd_loop()
{
        int ret = 0;
	struct timeval tv; 
	int fd = 0;


        do {
		net_socket_set_fds();
		memset( &tv, 0, sizeof( struct timeval ) );
		tv.tv_sec = socket_timeout; 
                ret = select( max_fd + 1 , &read_fds, NULL , NULL , &tv );

		if( ret > 0 )
		{
			for( fd = 0; fd <= max_fd; fd++ )
			{
				if( FD_ISSET( fd, &read_fds ) )
				{
					net_socket_read(fd);
				}
			}
		}
	} while( net_socket_shutdown == 0 );


	return ret;
}

void net_socket_loop_shutdown(int signal)
{
	int ret = 0;
	logging_printf(LOGGING_INFO, "net_socket_loop_shutdown: signal=%d action=shutdown\n", signal);
	set_shutdown_lock( 1 );
	ret = write( pipe_fd[0] , "X", 1 );
	ret = write( pipe_fd[1] , "X", 1 );
	close( pipe_fd[0] );
	close( pipe_fd[1] );
}

int net_socket_init( void )
{
	char *inbound_midi_filename = NULL;
	char *bind_address = NULL;
	int address_family = 0;
	int control_port, data_port, local_port;

	num_sockets = 0;
	max_fd = 0;
	FD_ZERO( &read_fds );

	control_port = config_int_get("network.control.port");
	data_port = config_int_get("network.data.port");
	local_port = config_int_get("network.local.port");

	bind_address = config_string_get("network.bind_address");
	get_sock_info( bind_address , control_port , NULL, NULL, &address_family);
	logging_printf(LOGGING_DEBUG, "net_socket_init: network.bind_address=[%s], family=%d\n", bind_address, address_family);

	switch( address_family )
	{
		case AF_INET: 
		case AF_INET6:
			if(
				net_socket_listener_create( address_family, bind_address, control_port ) ||
				net_socket_listener_create( address_family, bind_address, data_port ) ||
				net_socket_listener_create( address_family, bind_address, local_port ) )
			{
				logging_printf(LOGGING_ERROR, "net_socket_init: Cannot create socket: %s\n", strerror( errno ) );
				return -1;
			}
			break;
		default:
			logging_printf(LOGGING_ERROR, "net_socket_init: Invalid address family [%s][%d]\n", bind_address, address_family);
			return -1;
	}

	// If a file name is defined, open up the file handle to write inbound MIDI events
	inbound_midi_filename = config_string_get("inbound_midi");

	if( ! inbound_midi_filename )
	{
		logging_printf(LOGGING_WARN, "net_socket_setup: No filename defined for inbound_midi\n");
	} else if( ! check_file_security( inbound_midi_filename ) ) {
		logging_printf(LOGGING_WARN, "net_socket_setup: %s fails security check\n", inbound_midi_filename );
		logging_printf(LOGGING_WARN, "net_socket_setup: File mode=%s\n", config_string_get("file_mode") );
	} else {
		long file_mode_param = strtol( config_string_get("file_mode") , NULL, 8 );
		inbound_midi_fd = open( inbound_midi_filename, O_RDWR | O_CREAT , (mode_t)file_mode_param);
		
		if( inbound_midi_fd < 0 )
		{
			logging_printf(LOGGING_WARN, "net_socket_setup: Unable to open %s : %s\n", inbound_midi_filename, strerror( errno ) );
			inbound_midi_fd = -1;
		}
	}

#ifdef HAVE_ALSA
/* Add a dummy socket identifier to indicate that the listener loop should read from the ALSA input device */
	if( raveloxmidi_alsa_in_available() )
	{
		net_socket_add( RAVELOXMIDI_ALSA_INPUT );
	}
#endif

#ifdef HAVE_ALSA
	alsa_buffer_size = config_int_get("alsa.input_buffer_size");
	packet_size = MAX( NET_APPLEMIDI_UDPSIZE, alsa_buffer_size );
#else
	packet_size = NET_APPLEMIDI_UDPSIZE;
#endif

	packet = ( unsigned char * ) malloc( packet_size + 1);

	if( ! packet ) 
	{
		logging_printf(LOGGING_ERROR, "net_socket_init: Unable to allocate memory for read buffer\n");
		return -1;
	}
	return 0;
}

#ifdef HAVE_ALSA
static void * net_socket_alsa_listener( void *data )
{
	logging_printf(LOGGING_DEBUG, "net_socket_alsa_listener: Thread started\n");
	raveloxmidi_alsa_set_poll_fds( pipe_fd[0] );
	do {
		// Set poll timeout to be milliseconds
		if( raveloxmidi_alsa_poll( socket_timeout * 1000 ) == 1 )
		{
			net_socket_read( RAVELOXMIDI_ALSA_INPUT );
		}
	} while (net_socket_shutdown == 0 );
	logging_printf(LOGGING_DEBUG, "net_socket_alsa_listener: Thread stopped\n");

	return NULL;
}

int net_socket_alsa_loop()
{
	// Only start the thread if the input handle is available
	if( raveloxmidi_alsa_in_available() )
	{
		pthread_create( &alsa_listener_thread, NULL, net_socket_alsa_listener, NULL );
	}
	return 0;
}

void net_socket_wait_for_alsa(void)
{
	if( raveloxmidi_alsa_in_available() )
	{
		pthread_join( alsa_listener_thread, NULL );
	}
}

#endif

void net_socket_set_fds(void)
{
	int i = 0;

	max_fd = 0;

	FD_ZERO( &read_fds );
	for( i = 0; i < num_sockets; i++ )
	{
		if( sockets[i] > 0 )
		{
			FD_SET( sockets[i], &read_fds );
			max_fd = MAX( max_fd, sockets[i] );
		}
	}
}

int net_socket_get_data_socket( void )
{
	if( num_sockets <= 0 ) return -1;
	if( ! sockets ) return -1;

	return sockets[DATA_PORT];
}

int net_socket_get_control_socket( void )
{
	if( num_sockets <= 0 ) return -1;
	if( ! sockets ) return -1;

	return sockets[DATA_PORT - 1];
}

int net_socket_get_shutdown_fd( void )
{
	return pipe_fd[0];
}
