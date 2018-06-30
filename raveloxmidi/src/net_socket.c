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
#include "net_response.h"
#include "net_socket.h"
#include "net_connection.h"

#include "cmd_inv_handler.h"
#include "cmd_sync_handler.h"
#include "cmd_feedback_handler.h"
#include "cmd_end_handler.h"

#include "midi_note.h"
#include "rtp_packet.h"
#include "midi_command.h"
#include "midi_payload.h"
#include "utils.h"

#include "raveloxmidi_config.h"
#include "logging.h"

static int num_sockets;
static int *sockets;
static int net_socket_shutdown;
static int inbound_midi_fd = -1;

static pthread_mutex_t shutdown_lock;

int net_socket_create( unsigned int port )
{
	int new_socket;
	struct sockaddr_in socket_address;
	int *new_socket_list = NULL;

	new_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if( new_socket < 0 )
	{
		return errno;
	}

	memset(&(socket_address.sin_zero), 0, 8);    
	socket_address.sin_family = AF_INET;   
	socket_address.sin_addr.s_addr = htonl(INADDR_ANY);

	if (inet_aton( config_get("network.bind_address") , &(socket_address.sin_addr)) == 0) {
		logging_printf(LOGGING_ERROR, "net_socket_create: Invalid address: %s\n", config_get("network.bind_address") );
		return errno;
	}

	socket_address.sin_port = htons(port);
	if (bind(new_socket, (struct sockaddr *)&socket_address,
		sizeof(struct sockaddr)) < 0)
	{       
		return errno;
        } 

	num_sockets++;
	new_socket_list = (int *) realloc( sockets, sizeof(int) * num_sockets );
	if( ! new_socket_list )
	{
		logging_printf(LOGGING_ERROR, "net_socket_create: Insufficient memory to create socket %d\n", num_sockets );
		return -1;
	}
	sockets = new_socket_list;
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

	if( inbound_midi_fd >= 0 ) close(inbound_midi_fd);

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
	int ret = 0;

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

				logging_printf(LOGGING_ERROR, "net_socket_listener: Socket error (%d) on socket (%d)\n", errno , sockets[i] );
				break;
			}

			ip_address = inet_ntoa(from_addr.sin_addr);	
			logging_printf( LOGGING_DEBUG, "net_socket_listener: read(bytes=%u,socket=%d,host=%s,port=%u,first_byte=%02x)\n", recv_len, i,ip_address, ntohs( from_addr.sin_port ), packet[0]);

			// Apple MIDI command
			if( packet[0] == 0xff )
			{
				net_response_t *response = NULL;

				ret = net_applemidi_unpack( &command, packet, recv_len );
				net_applemidi_command_dump( command );

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
					size_t bytes_written = 0;
					bytes_written = sendto( sockets[i], response->buffer, response->len , 0 , (struct sockaddr *)&from_addr, from_len);
					logging_printf( LOGGING_DEBUG, "net_socket_listener: write(bytes=%u,socket=%d,host=%s,port=%u)\n", bytes_written, i,ip_address, ntohs( from_addr.sin_port ));	
					net_response_destroy( &response );
				}

				net_applemidi_cmd_destroy( &command );
			} else if( packet[0] == 0xaa )
			// MIDI note on internal socket
			{
				rtp_packet_t *rtp_packet = NULL;
				unsigned char *packed_rtp_buffer = NULL;
				size_t packed_rtp_buffer_len = 0;

				midi_note_t *midi_note = NULL;
				midi_payload_t *midi_payload = NULL;

				char *packed_journal = NULL;
				size_t packed_journal_len = 0;

				unsigned char *packed_payload = NULL;
				size_t packed_payload_len = 0;

				unsigned char *packed_rtp_payload = NULL;

                                midi_command_t *midi_commands=NULL;
                                size_t num_midi_commands=0;
                                size_t midi_command_index = 0;


				// FIX ME: Currently, the journal is the same for ALL contexts
				// FIX ME: The context ID is set to 0
				//
				// Get a journal if there is one
				net_ctx_journal_pack( 0 , &packed_journal, &packed_journal_len);

				// For the NOTE event, the MIDI note is already packed
				// but we still need to pack it into a payload
				// Create the payload
				midi_payload = midi_payload_create();

				if( midi_payload )
				{
					uint8_t ctx_id = 0;

					hex_dump( packet, recv_len );
					midi_payload_set_buffer( midi_payload, packet + 1 , recv_len - 1 );
					midi_payload_to_commands( midi_payload, MIDI_PAYLOAD_STREAM, &midi_commands, &num_midi_commands );

					if( packed_journal_len > 0 )
					{
						midi_payload_toggle_j( midi_payload );
					}
					
					midi_payload_pack( midi_payload, &packed_payload, &packed_payload_len );
					logging_printf(LOGGING_DEBUG, "packed_payload: buffer=%p,len=%u\n", packed_payload, packed_payload_len);
					hex_dump( packed_payload, packed_payload_len );

					// Join the packed MIDI payload and the journal together
					packed_rtp_payload = (unsigned char *)malloc( packed_payload_len + packed_journal_len );
					memcpy( packed_rtp_payload, packed_payload , packed_payload_len );
					memcpy( packed_rtp_payload + packed_payload_len , packed_journal, packed_journal_len );

					logging_printf(LOGGING_DEBUG, "packed_rtp_payload\n");
					hex_dump( packed_rtp_payload, packed_payload_len + packed_journal_len );

					// Build the RTP packet
					for( ctx_id = 0 ; ctx_id < _max_ctx ; ctx_id++ )
					{

						// Check that the connection id is active
						if( ! net_ctx_is_used( ctx_id ) ) continue;

						rtp_packet = rtp_packet_create();
						net_ctx_increment_seq( ctx_id );

						// Transfer the connection details to the RTP packet
						net_ctx_update_rtp_fields( ctx_id , rtp_packet );
	
						// Add the MIDI data to the RTP packet
						rtp_packet->payload_len = packed_payload_len + packed_journal_len;

						rtp_packet->payload = (unsigned char *)malloc( rtp_packet->payload_len );
						memcpy( rtp_packet->payload, packed_rtp_payload, rtp_packet->payload_len );
						rtp_packet_dump( rtp_packet );

						// Pack the RTP data
						rtp_packet_pack( rtp_packet, &packed_rtp_buffer, &packed_rtp_buffer_len );

						net_ctx_send( sockets[ DATA_PORT ], ctx_id , packed_rtp_buffer, packed_rtp_buffer_len );

						FREENULL( "packed_rtp_buffer", (void **)&packed_rtp_buffer );
						rtp_packet_destroy( &rtp_packet );
					}

					// Do some cleanup
					FREENULL( "packed_rtp_payload", (void **)&packed_rtp_payload );
					FREENULL( "packed_payload", (void **)&packed_payload );
					FREENULL( "packed_journal", (void **)&packed_journal );
					midi_payload_destroy( &midi_payload );

					ret = midi_note_unpack( &midi_note, packet + 1 , recv_len - 1);

					for( ctx_id = 0 ; ctx_id < _max_ctx ; ctx_id++ )
					{
						// Check that the connection id is active
						if( ! net_ctx_is_used( ctx_id ) ) continue;

						net_ctx_add_journal_note( ctx_id , midi_note );
					}
				}

				midi_note_destroy( &midi_note );

				for( ; num_midi_commands >= 1 ; num_midi_commands-- )
				{
					midi_command_reset( &(midi_commands[num_midi_commands - 1]) );
				}
				free( midi_commands );
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
				rtp_packet_dump( rtp_packet );

				midi_payload_unpack( &midi_payload, rtp_packet->payload, recv_len );

				// Read all the commands in the packet into an array
				midi_payload_to_commands( midi_payload, MIDI_PAYLOAD_RTP, &midi_commands, &num_midi_commands );

				// Sent a FEEBACK packet back to the originating host to ack the MIDI packet
				response = cmd_feedback_create( rtp_packet->header.ssrc, rtp_packet->header.seq );
                                if( response )
                                {
                                        size_t bytes_written = 0;
                                        bytes_written = sendto( sockets[i], response->buffer, response->len , 0 , (struct sockaddr *)&from_addr, from_len);
                                        logging_printf( LOGGING_DEBUG, "net_socket_listener: feedback write(bytes=%u,socket=%d,host=%s,port=%u)\n", bytes_written, i,ip_address, ntohs( from_addr.sin_port ));
                                        net_response_destroy( &response );
                                }

				// If an inbound midi file is defined, write the MIDI commands to it
				if( inbound_midi_fd >= 0 )
				{
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

							bytes_written = write( inbound_midi_fd, raw_buffer, 1 + midi_commands[midi_command_index].data_len );
							logging_printf( LOGGING_DEBUG, "net_socket_listener: inbound MIDI write(bytes=%u)\n", bytes_written );
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
	logging_printf(LOGGING_INFO, "net_socket_loop_shutdown: signal=%d action=shutdown\n", signal);
	set_shutdown_lock( 1 );
}

int net_socket_setup( void )
{
	num_sockets = 0;
	char *inbound_midi_filename = NULL;

	if(
		net_socket_create( atoi( config_get("network.control.port") ) ) ||
		net_socket_create( atoi( config_get("network.data.port") ) ) ||
		net_socket_create( atoi( config_get("network.note.port") ) ) )
	{
		logging_printf(LOGGING_ERROR, "net_socket_setup: Cannot create socket: %s\n", strerror( errno ) );
		return -1;
	}

	// If a file name is defined, open up the file handle to write inbound MIDI events
	inbound_midi_filename = config_get("inbound_midi");

	if( ! inbound_midi_filename )
	{
		logging_printf(LOGGING_WARN, "net_socket_setup: No filename defined for inbound_midi\n");
	} else if( ! check_file_security( inbound_midi_filename ) ) {
		logging_printf(LOGGING_WARN, "net_socket_setup: %s fails security check\n", inbound_midi_filename );
	} else {
		long file_mode_param = strtol( config_get("file_mode") , NULL, 8 );
		inbound_midi_fd = open( inbound_midi_filename, O_RDWR | O_CREAT , (mode_t)file_mode_param);
		
		if( inbound_midi_fd < 0 )
		{
			logging_printf(LOGGING_WARN, "net_socket_setup: Unable to open %s : %s\n", inbound_midi_filename, strerror( errno ) );
			inbound_midi_fd = -1;
		}
	}

	return 0;
}
