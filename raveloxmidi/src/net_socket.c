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
#include "data_table.h"

static data_table_t *sockets = NULL;

static int net_socket_shutdown = 0;
static int shutdown_fd[2] = {0,0};

static int inbound_midi_fd = -1;

static fd_set read_fds;
static int max_fd = 0;

static pthread_mutex_t shutdown_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t send_lock = PTHREAD_MUTEX_INITIALIZER;
int socket_timeout = 0;

static void net_socket_set_shutdown_lock( int i );

void net_socket_send_lock( void )
{
	pthread_mutex_lock( &send_lock );
}

void net_socket_send_unlock( void )
{
	pthread_mutex_unlock( &send_lock );
}

void net_socket_lock( raveloxmidi_socket_t *socket )
{
	if( ! socket ) return;
	pthread_mutex_lock( &(socket->lock) );
}

void net_socket_unlock( raveloxmidi_socket_t *socket )
{
	if(! socket ) return;
	pthread_mutex_unlock( &(socket->lock) );
}
	
void net_socket_dump( void *data )
{
	raveloxmidi_socket_t *socket = NULL;

	if( ! data ) return;

	socket = ( raveloxmidi_socket_t *)data;

	net_socket_lock( socket );
	logging_printf(LOGGING_DEBUG, "net_socket: fd=%d, packet_size=%u\n", socket->fd, socket->packet_size );
	net_socket_unlock( socket );
}

void net_socket_destroy( void **data )
{
	raveloxmidi_socket_t *socket = NULL;
	
	if(! data ) return;
	if(! *data ) return;

	logging_printf( LOGGING_DEBUG, "net_socket_destroy: data=%p\n", *data);

	socket = (raveloxmidi_socket_t *)(*data);
	if( ! socket ) return;

	net_socket_lock( socket );

	/* We don't destroy the ALSA handle as that gets cleaned up in the ALSA routines */
	/* so just NULL out the handle */
	if( socket->handle ) socket->handle = NULL;

	/* Only close the FD if it's a non-ALSA socket */
	if( socket->type == RAVELOXMIDI_SOCKET_FD_TYPE )
	{
		close( socket->fd );
	}

	if( socket->packet )
	{
		free( socket->packet );
		socket->packet = NULL;
	}

	if( socket->state )
	{
		midi_state_destroy( &( socket->state ) );
		socket->state = NULL;
	}

	net_socket_unlock( socket );
	pthread_mutex_destroy( &(socket->lock) );

	free( *data );
	*data = NULL;
}

static raveloxmidi_socket_t *net_socket_find_by_fd( int fd_to_find )
{
	size_t i = 0;
	size_t num_sockets = 0;

	num_sockets = data_table_item_count( sockets );

	if( num_sockets == 0 ) return NULL;

	for( i = 0; i < num_sockets; i++ )
	{
		raveloxmidi_socket_t *socket = NULL;

		socket = (raveloxmidi_socket_t *)data_table_item_get( sockets, i );

		if( socket )
		{
			if( socket->fd == fd_to_find )
			{
				return socket;
			}
		}
	}
	return NULL;
}

static raveloxmidi_socket_t *net_socket_create_item( void )
{
	raveloxmidi_socket_t *new_socket = NULL;
	midi_state_t *new_state = NULL;
#ifdef HAVE_ALSA
	size_t alsa_buffer_size = 0;
#endif
	size_t ring_buffer_size = 0;

	new_socket = (raveloxmidi_socket_t *)malloc( sizeof(raveloxmidi_socket_t) );

	if( ! new_socket )
	{
		logging_printf(LOGGING_ERROR, "net_socket_create_item: Insufficient memory to create new socket item\n");
		return NULL;
	}

	memset( new_socket, 0, sizeof( raveloxmidi_socket_t ) );

	new_socket->fd = 0;
	new_socket->packet = NULL;
	new_socket->type = RAVELOXMIDI_SOCKET_FD_TYPE;

#ifdef HAVE_ALSA
	alsa_buffer_size = config_int_get("alsa.input_buffer_size");
	new_socket->packet_size = MAX( NET_APPLEMIDI_UDPSIZE, alsa_buffer_size );
#else
	new_socket->packet_size = NET_APPLEMIDI_UDPSIZE;
#endif

	new_socket->packet = ( unsigned char * ) malloc( new_socket->packet_size + 1 );
	if( ! new_socket->packet )
	{
		logging_printf(LOGGING_ERROR, "net_socket_create_item: Insufficient memory to create socket buffer. Need %zu\n", new_socket->packet_size + 1);
		free( new_socket );
		new_socket = NULL;
	} else {
		ring_buffer_size = config_int_get("read.ring_buffer_size");
		ring_buffer_size = MAX( NET_SOCKET_DEFAULT_RING_BUFFER, ring_buffer_size );

		new_state = midi_state_create( ring_buffer_size );
		if( ! new_state )
		{
			logging_printf( LOGGING_ERROR, "net_socket_create_item: Insufficient memory to create midi state. Need %zu\n", ring_buffer_size );
			net_socket_destroy( (void **)&new_socket );
		} else {
			new_socket->state = new_state;
		}
	}

	return new_socket;
}

raveloxmidi_socket_t *net_socket_add( int new_socket_fd )
{
	raveloxmidi_socket_t **new_socket_list = NULL;
	raveloxmidi_socket_t *new_socket_item = NULL;

	if( new_socket_fd < 0 ) return NULL;

	new_socket_item = net_socket_create_item();
	if( ! new_socket_item ) return NULL;

	new_socket_item->fd = new_socket_fd;

	data_table_add_item( sockets, new_socket_item );

	return new_socket_item;
}

int net_socket_listener_create( int family, char *ip_address, unsigned int port )
{
	int new_socket = -1;
	struct sockaddr_in6 socket_address;
	socklen_t addr_len = 0;
	int optionvalue = 0;
	
	logging_printf(LOGGING_DEBUG, "net_socket_create: Creating socket for [%s]:%u, family=%d\n", ip_address, port, family);

	new_socket = socket(family, SOCK_DGRAM, IPPROTO_UDP);

	if( new_socket < 0 )
	{ 
		logging_printf( LOGGING_ERROR, "net_socket_listener_create: Unable to create socket %s:%u : %s\n", ip_address, port, strerror(errno));
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
	if( inbound_midi_fd >= 0 ) close(inbound_midi_fd);

	data_table_destroy( &sockets );

	return 0;
}

int net_socket_read( int fd )
{
	ssize_t recv_len = 0;
	unsigned from_len = 0;
	struct sockaddr_storage from_addr;
	int output_enabled = 0;
	char ip_address[ INET6_ADDRSTRLEN ];
	uint16_t from_port = 0;
	net_applemidi_command *command;
	int ret = 0;
	int data_fd = 0;
	int control_fd = 0;
	int local_fd = 0;
	unsigned char *packet = NULL;
	size_t packet_size = 0;
	raveloxmidi_socket_t *found_socket = NULL;

	char *read_buffer = NULL;
	size_t read_buffer_size = 0;

	net_socket_send_lock();
	data_fd = net_socket_get_data_socket();
	control_fd = net_socket_get_control_socket();
	local_fd = net_socket_get_local_socket();
	net_socket_send_unlock();

	memset( ip_address, 0, INET6_ADDRSTRLEN );
	from_len = sizeof( from_addr );

	found_socket = net_socket_find_by_fd( fd );

	if( ! found_socket )
	{
		logging_printf(LOGGING_ERROR, "net_socket_read: Cannot locate fd=%d\n", fd );
		return 0;
	}

	packet = found_socket->packet;
	packet_size = found_socket->packet_size;

	if( fd == data_fd )
	{
		logging_printf(LOGGING_INFO, "net_socket_read: data_fd\n");
	} else if (fd == control_fd ) {
		logging_printf(LOGGING_INFO, "net_socket_read: control_fd\n");
	} else if (fd == local_fd ) {
		logging_printf(LOGGING_INFO, "net_socket_read: local_fd\n");
	}

#ifdef HAVE_ALSA
	if( found_socket->type == RAVELOXMIDI_SOCKET_ALSA_TYPE )
	{
		logging_printf(LOGGING_INFO, "net_socket_read: alsa handle\n");
	} 
#endif

	

	while( 1 )
	{
		memset( packet, 0, packet_size + 1 );
#ifdef HAVE_ALSA
		if( found_socket->type  == RAVELOXMIDI_SOCKET_ALSA_TYPE )
		{
			recv_len = raveloxmidi_alsa_read( found_socket->handle, packet, packet_size);
		} else {
#endif
			recv_len = recvfrom( fd, packet, NET_APPLEMIDI_UDPSIZE, 0, (struct sockaddr *)&from_addr, &from_len );
			get_ip_string( (struct sockaddr *)&from_addr, ip_address, INET6_ADDRSTRLEN );
			from_port = ntohs( ((struct sockaddr_in *)&from_addr)->sin_port );
#ifdef HAVE_ALSA
		}
		if( found_socket->type  != RAVELOXMIDI_SOCKET_ALSA_TYPE )
		{
#endif
			if( recv_len > 0 )
			{
				logging_printf( LOGGING_DEBUG, "net_socket_read: read socket=%d, recv_len=%ld, host=%s, port=%u, first_byte=%02x)\n", fd, recv_len, ip_address, from_port, packet[0]);
			}

#ifdef HAVE_ALSA
		} else {
			if( recv_len > 0 )
			{
				logging_printf( LOGGING_DEBUG, "net_socket_read: read socket=ALSA(%d) recv_len=%ld first_byte=%02x\n", fd, recv_len, packet[0] );
			}
		}
#endif
		if ( recv_len <= 0) break;

		midi_state_write( found_socket->state, packet, recv_len );

	}

	// Apple MIDI command
	if( ( ( fd == control_fd ) || ( fd == data_fd ) ) && ( midi_state_char_compare( found_socket->state, 0xff, 0 ) == 1 ) )
	{
		net_response_t *response = NULL;

		read_buffer = midi_state_drain( found_socket->state, &read_buffer_size );

		hex_dump( read_buffer, read_buffer_size );

		ret = net_applemidi_unpack( &command, read_buffer, read_buffer_size );
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
				applemidi_by_responder( command->data );
				break;
			case NET_APPLEMIDI_CMD_SYNC:
				response = applemidi_sync_responder( command->data );
				break;
			case NET_APPLEMIDI_CMD_FEEDBACK:
				applemidi_feedback_responder( command->data );
				break;
			case NET_APPLEMIDI_CMD_BITRATE:
				break;
		}

		if( response )
		{
			size_t bytes_written = 0;
			net_socket_send_lock();
			bytes_written = sendto( fd, response->buffer, response->len , MSG_CONFIRM, (void *)&from_addr, from_len);
			net_socket_send_unlock();
			logging_printf( LOGGING_DEBUG, "net_socket_read: response write(bytes=%u,socket=%d,host=%s,port=%u)\n", bytes_written, fd,ip_address, from_port );	
			net_response_destroy( &response );
		}

		net_applemidi_cmd_destroy( &command );
	// Heartbeat request
	} else if( ( fd == local_fd ) && ( midi_state_compare( found_socket->state, "STAT", 4) == 0) )
	{
		const char *buffer="OK";
		size_t bytes_written = 0;

		net_socket_send_lock();
		bytes_written = sendto( fd, buffer, strlen(buffer), MSG_CONFIRM, (void *)&from_addr, from_len);
		net_socket_send_unlock();

		logging_printf(LOGGING_DEBUG, "net_socket_read: Heartbeat request. Response written: %u\n", bytes_written);
		midi_state_advance( found_socket->state, 4);
	// Shutdown request
	} else if( ( fd == local_fd ) && ( midi_state_compare( found_socket->state, "QUIT", 4) == 0 ) )
	{
		int ret = 0;
		const char *buffer="QT";
		size_t bytes_written = 0;

		net_socket_send_lock();
		bytes_written = sendto( fd, buffer, strlen(buffer), MSG_CONFIRM, (void *)&from_addr, from_len);
		net_socket_send_unlock();

		logging_printf(LOGGING_DEBUG, "net_socket_read: Shutdown request. Response written: %u\n", bytes_written);
		logging_printf(LOGGING_NORMAL, "Shutdown request received on local socket\n");

		net_socket_set_shutdown_lock(1);

		ret = write( shutdown_fd[0] , "X", 1 );
		ret = write( shutdown_fd[1] , "X", 1 );
	// Connection list request
	} else if( ( fd == local_fd ) && ( midi_state_compare( found_socket->state, "LIST", 4) == 0 ) )
	{
		int ret = 0;
		char *buffer = NULL;
		size_t bytes_written = 0;

		buffer = net_ctx_connections_to_string();
		if( buffer )
		{
			hex_dump( buffer, strlen( buffer ) );

			net_socket_send_lock();
			bytes_written = sendto( fd, (const char *)buffer, strlen(buffer), MSG_CONFIRM, (void *)&from_addr, from_len);
			net_socket_send_unlock();

			free( buffer );
		}

		logging_printf(LOGGING_DEBUG, "net_socket_read: List request. Response written: %u\n", bytes_written);

#ifdef HAVE_ALSA
	} else if( ( fd == local_fd ) || (found_socket->type==RAVELOXMIDI_SOCKET_ALSA_TYPE) )
#else
	} else if( fd == local_fd )
#endif
	// MIDI data on internal socket or ALSA rawmidi device
	{
		midi_state_dump( found_socket->state );

		read_buffer = midi_state_drain( found_socket->state, &read_buffer_size );
		net_distribute_midi( read_buffer, read_buffer_size );
	} else {
	// RTP MIDI inbound from remote socket
		rtp_packet_t *rtp_packet = NULL;
		midi_payload_t *midi_payload=NULL;
		midi_command_t *midi_commands=NULL;
		size_t num_midi_commands=0;
		net_response_t *response = NULL;
		size_t midi_command_index = 0;

		read_buffer = midi_state_drain( found_socket->state, &read_buffer_size );

		rtp_packet = rtp_packet_create();
		rtp_packet_unpack( read_buffer, read_buffer_size, rtp_packet );
		logging_printf(LOGGING_DEBUG, "net_socket_read: inbound MIDI received\n");
		rtp_packet_dump( rtp_packet );

		midi_payload_unpack( &midi_payload, rtp_packet->payload, read_buffer_size );

		// Read all the commands in the packet into an array
		midi_payload_to_commands( midi_payload, MIDI_PAYLOAD_RTP, &midi_commands, &num_midi_commands );

		// Sent a FEEBACK packet back to the originating host to ack the MIDI packet
		response = applemidi_feedback_create( rtp_packet->header.ssrc, rtp_packet->header.seq );
		if( response )
		{
			size_t bytes_written = 0;
			net_socket_send_lock();
			bytes_written = sendto( fd, response->buffer, response->len , MSG_CONFIRM, (void *)&from_addr, from_len);
			net_socket_send_unlock();
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
						net_socket_send_lock();
						bytes_written = write( inbound_midi_fd, raw_buffer, 1 + midi_commands[midi_command_index].data_len );
						net_socket_send_unlock();
						logging_printf( LOGGING_DEBUG, "net_socket_read: inbound MIDI write(bytes=%u)\n", bytes_written );
					}

#ifdef HAVE_ALSA
					net_socket_send_lock();
					raveloxmidi_alsa_write( raw_buffer, 1 + midi_commands[midi_command_index].data_len );
					net_socket_send_unlock();
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

	free( read_buffer );
	return ret;
}

static void net_socket_set_shutdown_lock( int i )
{
	pthread_mutex_lock( &shutdown_lock );
	logging_printf(LOGGING_DEBUG,"net_socket_set_shutdown_lock: %d\n", i);
	net_socket_shutdown = i;
	logging_printf(LOGGING_DEBUG,"net_socket_set_shutdown_lock: exit\n");
	pthread_mutex_unlock( &shutdown_lock );
}

int net_socket_get_shutdown_status( void )
{
	int i = OK;

	pthread_mutex_lock( &shutdown_lock );
	i = net_socket_shutdown;
	logging_printf(LOGGING_DEBUG, "net_socket_get_shutdown_status: %d\n", i);
	pthread_mutex_unlock( &shutdown_lock );
	return i;
}

void net_socket_loop_init()
{
	int err = 0;

	pthread_mutex_init( &shutdown_lock, NULL);
	net_socket_set_shutdown_lock(0);
	socket_timeout = config_long_get("network.socket_timeout");

// Set up a pipe as a shutdown signal
	err=pipe( shutdown_fd );
	if( err < 0 )
	{
		logging_printf( LOGGING_ERROR, "net_socket_loop_init: pipe error: %s\n", strerror(errno));
	} else {
		logging_printf(LOGGING_DEBUG, "net_socket_loop_init: pipe0=%d pipe1=%d\n", shutdown_fd[0], shutdown_fd[1]);
		FD_SET( shutdown_fd[1], &read_fds );
	}
	fcntl(shutdown_fd[0], F_SETFL, O_NONBLOCK);
	fcntl(shutdown_fd[1], F_SETFL, O_NONBLOCK);
}

void net_socket_loop_teardown()
{
	pthread_mutex_destroy( &shutdown_lock );
}

int net_socket_fd_loop()
{
        int ret = 0;
	struct timeval tv; 
	int fd = 0;


        do {
		net_socket_send_lock();
		net_socket_set_fds();
		net_socket_send_unlock();
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
	} while( net_socket_get_shutdown_status() == OK );


	return ret;
}

void net_socket_loop_shutdown(int signal)
{
	int ret = 0;
	logging_printf(LOGGING_INFO, "net_socket_loop_shutdown: signal=%d action=shutdown\n", signal);
	net_socket_set_shutdown_lock( 1 );
	ret = write( shutdown_fd[0] , "X", 1 );
	ret = write( shutdown_fd[1] , "X", 1 );
	close( shutdown_fd[0] );
	close( shutdown_fd[1] );
}

int net_socket_init( void )
{
	char *inbound_midi_filename = NULL;
	char *bind_address = NULL;
	int address_family = 0;
	int control_port, data_port, local_port;

	pthread_mutex_init( &send_lock, NULL );

	max_fd = 0;
	FD_ZERO( &read_fds );

	sockets = data_table_create("sockets", net_socket_destroy, net_socket_dump );

	if( ! sockets ) 
	{
		logging_printf( LOGGING_ERROR, "net_socket_init: Unable to create sockets table\n");
		return -1;
	}

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

	return 0;
}

void net_socket_set_fds(void)
{
	int i = 0;
	size_t num_sockets = 0;

	max_fd = 0;

	num_sockets = data_table_item_count( sockets );

	if( num_sockets == 0 ) return;

	FD_ZERO( &read_fds );
	for( i = 0; i < num_sockets; i++ )
	{
		raveloxmidi_socket_t *socket = NULL;

		socket = (raveloxmidi_socket_t *)data_table_item_get( sockets, i );

		if( ! socket ) continue;
		if( socket->fd > 0 )
		{
			FD_SET( socket->fd, &read_fds );
			max_fd = MAX( max_fd, socket->fd );
		}
	}
}

int net_socket_get_data_socket( void )
{
	int fd = -1;
	raveloxmidi_socket_t *socket = NULL;

	socket = (raveloxmidi_socket_t *)data_table_item_get( sockets, NET_SOCKET_DATA_PORT);

	if( socket )
	{
		fd =  socket->fd;
	}

	return fd;
}

int net_socket_get_control_socket( void )
{
	int fd = -1;
	raveloxmidi_socket_t *socket = NULL;

	socket = (raveloxmidi_socket_t *)data_table_item_get( sockets, NET_SOCKET_CONTROL_PORT);

	if( socket )
	{
		fd =  socket->fd;
	}

	return fd;
}

int net_socket_get_local_socket( void )
{
	int fd = -1;
	raveloxmidi_socket_t *socket = NULL;

	socket = (raveloxmidi_socket_t *)data_table_item_get( sockets, NET_SOCKET_LOCAL_PORT);

	if( socket )
	{
		fd =  socket->fd;
	}

	return fd;
}

int net_socket_get_shutdown_fd( void )
{
	return shutdown_fd[0];
}
