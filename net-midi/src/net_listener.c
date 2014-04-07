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
#include "cmd_end_handler.h"

#include "midi_note_packet.h"
#include "rtp_packet.h"
#include "midi_payload.h"
#include "utils.h"

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

				fprintf(stderr, "Socket error (%d) on socket (%d)\n", errno , sockets[i] );
				break;
			}

			ip_address = inet_ntoa(from_addr.sin_addr);	

			// Determine the packet type

			fprintf(stderr, "Socket: %u\n", sockets[i]);
			fprintf(stderr, "Recv Length = %u\n", recv_len );
			fprintf(stderr, "Byte[0] == %02x\n", packet[0]);

			hex_dump( packet, recv_len );

			fflush( stderr );
			// Apple MIDI command
			if( packet[0] == 0xff )
			{
				net_response_t *response = NULL;

				ret = net_applemidi_unpack( &command, packet, recv_len );

				switch( command->command )
				{
					case NET_APPLEMIDI_CMD_INV:
						response = cmd_inv_handler( ip_address, from_addr.sin_port, command->data );
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
						break;
					case NET_APPLEMIDI_CMD_BITRATE:
						break;
						;;
				}

				if( response )
				{
					hex_dump( response->buffer, response->len );
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
				size_t packed_rtp_payload_len = 0;

				fprintf(stderr, "Connection on MIDI note port\n");
				ret = midi_note_packet_unpack( &note_packet, packet + 1 , recv_len - 1);

				// DEBUG
				midi_note_packet_dump( note_packet );

				// NOTE ON
				// Get a journal if there is one
				net_ctx_journal_pack( 0 , &packed_journal, &packed_journal_len);
				// For the NOTE ON event, the MIDI note is already packed
				// but we still need to pack it into a payload
				// Create the payload
				midi_payload = midi_payload_create();

				if( midi_payload )
				{
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

					// Do some cleanup
					FREENULL( (void **)&packed_payload );
					FREENULL( (void **)&packed_journal );
					midi_payload_destroy( &midi_payload );

					// Build the RTP packet
					rtp_packet = rtp_packet_create();
					rtp_packet_dump( rtp_packet );
					net_ctx_update_rtp_fields( 0 , rtp_packet );
					rtp_packet_dump( rtp_packet );
	
					// Add the MIDI data to the RTP packet
					rtp_packet->payload_len = packed_payload_len + packed_journal_len;
					rtp_packet->payload = packed_rtp_payload;

					// Pack the RTP data
					rtp_packet_pack( rtp_packet, &packed_rtp_buffer, &packed_rtp_buffer_len );

					// Send the RTP packet
					// TODO:
					hex_dump( packed_rtp_buffer, packed_rtp_buffer_len );

					// Clean up
					FREENULL( (void **)&packed_payload );
					rtp_packet_destroy( &rtp_packet );

					fprintf(stderr, "Adding note to journal\n");
					net_ctx_add_journal_note( 0 , note_packet->channel + 1 , note_packet->note, note_packet->velocity );
				}

				// Add the NoteOff event for the same note
				net_ctx_add_journal_note( 0 , note_packet->channel + 1, note_packet->note, 0 );

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
	fprintf(stderr, "Received signal(%d) shutting down\n", signal);
	set_shutdown_lock( 1 );
}

int net_socket_setup( void )
{
	int i;
	int ret;

	num_sockets = 0;

	for( i = 5004 ; i <= 5006 ; i++ )
	{
		ret = net_socket_create( i );
		if( ret != 0 )
		{
			fprintf(stderr, "Cannot create socket on port %u: %s\n", i, strerror( errno ) );
			return -1;
		}
	}

	return 0;
}
