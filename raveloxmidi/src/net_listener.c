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

#include "net_applemidi.h"
#include "net_listener.h"
#include "net_connection.h"

#include "cmd_inv_handler.h"
#include "cmd_sync_handler.h"
#include "cmd_feedback_handler.h"
#include "cmd_end_handler.h"

#include "midi_note_packet.h"
#include "rtp_packet.h"
#include "midi_payload.h"
#include "utils.h"

#include "raveloxmidi_config.h"
#include "logging.h"

static int num_sockets;
static int *sockets;
static int net_socket_shutdown;

static pthread_mutex_t shutdown_lock;

net_response_t * new_net_response( void )
{
	net_response_t *response = NULL;

	response =( net_response_t *) malloc( sizeof( net_response_t ) );

	if( response )
	{
		response->buffer = NULL;
		response->len = 0;
	}

	return response;
}

void net_response_destroy( net_response_t **response )
{
	if( ! *response ) return;

	if( (*response)->buffer )
	{
		free( (*response)->buffer );
		(*response)->buffer = NULL;
	}

	free( *response );
	*response = NULL;
}

int net_socket_create( unsigned int port )
{
	int new_socket;
	struct sockaddr_in socket_address;

	new_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if( new_socket < 0 )
	{
		return errno;
	}

	memset(&(socket_address.sin_zero), 0, 8);    
	socket_address.sin_family = AF_INET;   
	socket_address.sin_addr.s_addr = htonl(INADDR_ANY);

	if (inet_aton( config_get("network.bind_address") , &(socket_address.sin_addr)) == 0) {
		logging_printf(LOGGING_ERROR, "Invalid address: %s\n", config_get("network.bind_address") );
		return errno;
	}

	socket_address.sin_port = htons(port);
	if (bind(new_socket, (struct sockaddr *)&socket_address,
		sizeof(struct sockaddr)) < 0)
	{       
		return errno;
        } 

	num_sockets++;
	sockets = (int *)realloc( sockets , sizeof(int) * num_sockets );
	sockets[num_sockets - 1 ] = new_socket;

	fcntl(new_socket, F_SETFL, O_NONBLOCK);

	return 0;
}

int net_socket_destroy( void )
{
	int socket;

	for(socket = 0 ; socket < num_sockets ; socket++ )
	{
		close( sockets[socket] );
	}

	free(sockets);

	return 0;
}

int net_socket_listener( void )
{
	unsigned char packet[ NET_APPLEMIDI_UDPSIZE + 1 ];
	int i;
	int recv_len;
	unsigned from_len;
	struct sockaddr_in from_addr;
	char *ip_address = NULL;

	net_applemidi_command *command;
	int ret;

	from_len = sizeof( struct sockaddr );
	for( i = 0 ; i < num_sockets ; i++ )
	{

		while( 1 )
		{
			recv_len = recvfrom( sockets[ i ], packet, NET_APPLEMIDI_UDPSIZE, 0,  (struct sockaddr *)&from_addr, &from_len );

			if ( recv_len <= 0)
			{   
				if ( errno == EAGAIN )
				{   
					break;
				}   

				logging_printf(LOGGING_ERROR, "Socket error (%d) on socket (%d)\n", errno , sockets[i] );
				break;
			}

			ip_address = inet_ntoa(from_addr.sin_addr);	
			logging_printf( LOGGING_DEBUG, "Data (0x%02x) on socket(%d) from (%s)\n", packet[0], i,ip_address);

			// Determine the packet type

			// Apple MIDI command
			if( packet[0] == 0xff )
			{
				net_response_t *response = NULL;

				ret = net_applemidi_unpack( &command, packet, recv_len );

				switch( command->command )
				{
					case NET_APPLEMIDI_CMD_INV:
						response = cmd_inv_handler( ip_address, ntohs( from_addr.sin_port ), command->data );
						break;
					case NET_APPLEMIDI_CMD_ACCEPT:
						break;
					case NET_APPLEMIDI_CMD_REJECT:
						break;
					case NET_APPLEMIDI_CMD_END:
						response = cmd_end_handler( command->data );
						break;
					case NET_APPLEMIDI_CMD_SYNC:
						response = cmd_sync_handler( command->data );
						break;
					case NET_APPLEMIDI_CMD_FEEDBACK:
						response = cmd_feedback_handler( command->data );
						break;
					case NET_APPLEMIDI_CMD_BITRATE:
						break;
						;;
				}

				if( response )
				{
					sendto( sockets[i], response->buffer, response->len , 0 , (struct sockaddr *)&from_addr, from_len);
					net_response_destroy( &response );
				}

				net_applemidi_cmd_destroy( &command );
			}

			// MIDI note from sending device
			if( packet[0] == 0xaa )
			{
				rtp_packet_t *rtp_packet = NULL;
				unsigned char *packed_rtp_buffer = NULL;
				size_t packed_rtp_buffer_len = 0;

				midi_note_packet_t *note_packet = NULL;
				midi_payload_t *midi_payload = NULL;

				char *packed_journal = NULL;
				size_t packed_journal_len = 0;

				unsigned char *packed_payload = NULL;
				size_t packed_payload_len = 0;

				unsigned char *packed_rtp_payload = NULL;


				// Get a journal if there is one
				net_ctx_journal_pack( 0 , &packed_journal, &packed_journal_len);

				// For the NOTE event, the MIDI note is already packed
				// but we still need to pack it into a payload
				// Create the payload
				midi_payload = midi_payload_create();

				if( midi_payload )
				{
					uint8_t ctx_id = 0;

					payload_set_buffer( midi_payload, packet + 1 , recv_len - 1 );

					if( packed_journal_len > 0 )
					{
						payload_toggle_j( midi_payload );
					}
					
					payload_pack( midi_payload, &packed_payload, &packed_payload_len );

					// Join the packed MIDI payload and the journal together
					packed_rtp_payload = (unsigned char *)malloc( packed_payload_len + packed_journal_len );
					memcpy( packed_rtp_payload, packed_payload , packed_payload_len );
					memcpy( packed_rtp_payload + packed_payload_len , packed_journal, packed_journal_len );

					// Build the RTP packet
					for( ctx_id = 0 ; ctx_id < _max_ctx ; ctx_id++ )
					{

						rtp_packet = rtp_packet_create();
						net_ctx_increment_seq( ctx_id );

						// Transfer the connection details to the RTP packet
						net_ctx_update_rtp_fields( ctx_id , rtp_packet );
	
						// Add the MIDI data to the RTP packet
						rtp_packet->payload_len = packed_payload_len + packed_journal_len;
						rtp_packet->payload = packed_rtp_payload;

						rtp_packet_dump( rtp_packet );

						// Pack the RTP data
						rtp_packet_pack( rtp_packet, &packed_rtp_buffer, &packed_rtp_buffer_len );

						net_ctx_send( ctx_id , packed_rtp_buffer, packed_rtp_buffer_len );

						FREENULL( (void **)&packed_rtp_buffer );
						rtp_packet_destroy( &rtp_packet );
					}

					// Do some cleanup
					FREENULL( (void **)&packed_rtp_payload );
					FREENULL( (void **)&packed_payload );
					FREENULL( (void **)&packed_journal );
					midi_payload_destroy( &midi_payload );

					ret = midi_note_packet_unpack( &note_packet, packet + 1 , recv_len - 1);

					for( ctx_id = 0 ; ctx_id < _max_ctx ; ctx_id++ )
					{
						// Check that the connection id is active
						if( ! net_ctx_is_used( ctx_id ) ) continue;

						net_ctx_add_journal_note( ctx_id , note_packet );
					}
				}

				midi_note_packet_destroy( &note_packet );
			}
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

static int get_shutdown_lock ( void )
{
	int i = 0;
	pthread_mutex_lock( &shutdown_lock );
	i = net_socket_shutdown;
	pthread_mutex_unlock( &shutdown_lock );
	return i;
}

int net_socket_loop( unsigned int interval )
{
        struct timeval tv; 
        int ret = 0;

	pthread_mutex_init( &shutdown_lock , NULL );

	set_shutdown_lock( 0 );
        do {
                tv.tv_sec = 0;
                tv.tv_usec = interval;
                ret = select( 0 , NULL , NULL , NULL , &tv );

		net_socket_listener();

	} while( get_shutdown_lock() == 0 );

	pthread_mutex_destroy( &shutdown_lock );

	return ret;
}

void net_socket_loop_shutdown(int signal)
{
	logging_printf(LOGGING_INFO, "Received signal(%d) shutting down\n", signal);
	set_shutdown_lock( 1 );
}

int net_socket_setup( void )
{
	num_sockets = 0;

	if(
		net_socket_create( atoi( config_get("network.rtpmidi.port") ) ) ||
		net_socket_create( atoi( config_get("network.rtsp.port") ) ) ||
		net_socket_create( atoi( config_get("network.note.port") ) ) )
	{
		logging_printf(LOGGING_ERROR, "Cannot create socket: %s\n", strerror( errno ) );
		return -1;
	}

	return 0;
}
