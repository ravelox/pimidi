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

#include <errno.h>
extern int errno;

#include "net_applemidi.h"
#include "net_listener.h"
#include "net_connection.h"

#include "cmd_inv_handler.h"
#include "cmd_sync_handler.h"
#include "cmd_end_handler.h"

#include "midi_note_packet.h"
#include "utils.h"

static int control_socket, data_socket, midi_socket;
static int num_sockets;
static int *sockets;
static int net_socket_shutdown;

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

	return new_socket;
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
			fprintf(stderr, "Byte[0] == %u\n", packet[0]);

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
				midi_note_packet_t *note_packet;

				fprintf(stderr, "Connection on MIDI note port\n");
				ret = midi_note_packet_unpack( &note_packet, packet, recv_len );

				midi_note_packet_dump( note_packet );

				midi_note_packet_destroy( &note_packet );
			}
		}
	}
	return ret;
}

int net_socket_loop( unsigned int interval )
{
        struct timeval tv; 
        int ret = 0;

	net_socket_shutdown = 0;
        do {
                tv.tv_sec = 0;
                tv.tv_usec = interval;
                ret = select( 0 , NULL , NULL , NULL , &tv );

		net_socket_listener();

	} while( net_socket_shutdown == 0 );

	return ret;
}

void net_socket_loop_shutdown(int signal)
{
	fprintf(stderr, "Received signal(%d) shutting down\n", signal);
	net_socket_shutdown = 1;
}

int net_socket_setup( void )
{
	int i;

	num_sockets = 0;
	control_socket = net_socket_create( 5004 );
	printf("Control Socket: %d\n", control_socket);
	data_socket = net_socket_create( 5005 );
	printf("Data Socket: %d\n", data_socket);
	midi_socket = net_socket_create( 5006 );
	printf("MIDI Socket: %d\n", midi_socket);

	for( i = 0 ; i < num_sockets ; i++ )
	{
		printf("Socket(%d)=%d\n", i, sockets[i]);
	}
	return 0;
}
