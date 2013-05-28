#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <signal.h>

#include "net_applemidi.h"
#include "net_listener.h"
#include "midi_journal.h"
#include "net_connection.h"
#include "utils.h"

void show_command( net_applemidi_command *command)
{

		if( ! command )
		{
			fprintf(stderr, "Null command\n");
			return;
		}

		printf("Packet:\n");
		printf("Signature: 0x%04x\n", command->signature);

		switch( command->command )
		{
			case NET_APPLEMIDI_CMD_INV:
				printf("Command: IN\n");
				break;
			case NET_APPLEMIDI_CMD_END:
				printf("Command: BY\n");
				break;
			case NET_APPLEMIDI_CMD_ACCEPT:
				printf("Command: OK\n");
				break;
			case NET_APPLEMIDI_CMD_REJECT:
				printf("Command: NO\n");
				break;
			case NET_APPLEMIDI_CMD_FEEDBACK:
				printf("Command: RS\n");
				break;
			case NET_APPLEMIDI_CMD_BITRATE:
				printf("Command: RL\n");
				break;
			case NET_APPLEMIDI_CMD_SYNC:
				printf("Command: CK\n");
				break;
		}


		if( command->command == NET_APPLEMIDI_CMD_INV )
		{
			net_applemidi_inv	*inv_data;
			inv_data = (net_applemidi_inv *)command->data;
			printf("inv_data:\n");
			printf("Version  : %u\n", inv_data->version);
			printf("Initiator: 0x%08x\n", inv_data->initiator);
			printf("SSRC     : 0x%08x\n", inv_data->ssrc);
			printf("Name     : %s\n", inv_data->name);
		}

		if( command->command == NET_APPLEMIDI_CMD_SYNC )
		{
			net_applemidi_sync	*sync_data;
			sync_data = (net_applemidi_sync *)command->data;
			printf("sync_data:\n");
			printf("SSRC	   : 0x%08x\n", sync_data->ssrc);
			printf("Count	   : %d\n", sync_data->count);
			printf("Padding    : 0x%02x%02x%02x\n",
				sync_data->padding[0], sync_data->padding[1], sync_data->padding[2] );
			printf("Timestamp 1: 0x%016llx\n", sync_data->timestamp1 );
			printf("Timestamp 2: 0x%016llx\n", sync_data->timestamp2 );
			printf("Timestamp 3: 0x%016llx\n", sync_data->timestamp3 );
		}

		if( command->command == NET_APPLEMIDI_CMD_FEEDBACK )
		{
			net_applemidi_feedback	*feedback_data;
			feedback_data = (net_applemidi_feedback *)command->data;
			printf("feedback_data:\n");
			printf("SSRC	     : 0x%08x\n", feedback_data->ssrc);
			printf("Apple seq num: %u\n", feedback_data->apple_seq);
			printf("RTP seq num  : %u\n", feedback_data->rtp_seq[1]);
		}
}

int main(int argc, char *argv[])
{
	/* Invitation buffer */
	 unsigned char buffer[] = { 0xff, 0xff, 0x49, 0x4e, 0x00, 0x00, 0x00, 0x02, 0x66, 0x33, 0x48, 0x73, 0x9e, 0xc3, 0xdb, 0xe3, 0x41, 0x43, 0x37, 0x00 };
	 size_t buffer_len = 20;

	/* Sync buffer */
	// unsigned char buffer[] = { 0xff, 0xff, 0x43, 0x4b, 0xa3, 0xa2, 0x02, 0xf2, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x33, 0x38, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x12, 0x84, 0xe8, 0xe8, 0x3e, 0x01, 0xe0, 0x42, 0x95, 0x7b };
	// size_t buffer_len = 36;

	/* Feedback buffer */
	// unsigned char buffer[] = { 0xff, 0xff, 0x52, 0x53, 0x1c, 0x03, 0x73, 0x94, 0xd2, 0x4b, 0xa5, 0x7b };
	// size_t buffer_len = 12;

	/* End buffer */
	// unsigned char buffer[] = { 0xff, 0xff, 0x42, 0x59, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0xbd, 0x06, 0x09, 0x0b };
	// size_t buffer_len = 16;

	net_applemidi_command *command;
	int result;
	unsigned char *pack_buffer;
	size_t pack_buffer_len;

	printf("Variable sizes are:\n");
	printf("\tuint8_t = %u\n", sizeof( uint8_t ) );
	printf("\tuint16_t= %lu\n", sizeof( uint16_t ) );
	printf("\tuint32_t= %lu\n", sizeof( uint32_t ) );
	printf("\tuint64_t= %lu\n", sizeof( uint64_t ) );
	printf("\tvoid *  = %lu\n", sizeof( void * ) );
	printf("\tchar *  = %lu\n", sizeof( char * ) );
	result = net_applemidi_unpack( &command, buffer, buffer_len );
	printf("\n\nUnpack result = %u\n", result);

	if( result == NET_APPLEMIDI_DONE )
	{
		show_command( command );
	}

	result = net_applemidi_pack( command, &pack_buffer, &pack_buffer_len );

	printf("\n\nPack result = %u\n", result);
	printf("Pack length = %zu\n", pack_buffer_len);
	hex_dump( pack_buffer, pack_buffer_len );
	hex_dump( buffer, buffer_len );
	
	net_applemidi_cmd_destroy( &command );

	result = net_applemidi_unpack( &command, pack_buffer, pack_buffer_len );
	if( result == NET_APPLEMIDI_DONE )
	{
		show_command( command );
		net_applemidi_cmd_destroy( &command );
		free( pack_buffer );
	}

	net_socket_setup();
	net_ctx_init();

        signal( SIGINT , net_socket_loop_shutdown);
        signal( SIGUSR2 , net_socket_loop_shutdown);

	net_socket_loop( 5000 );

	net_socket_destroy();
	net_ctx_destroy();

	return 0;
}
