#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include <arpa/inet.h>

#include "midi_note_packet.h"
#include "utils.h"

midi_note_packet_t * midi_note_packet_create( void )
{
	midi_note_packet_t *packet;

	packet = (midi_note_packet_t *) malloc( sizeof( midi_note_packet_t ) );

	if( ! packet )  return NULL;

	memset( packet, 0, sizeof( midi_note_packet_t ) );

	return packet;
}

void midi_note_packet_destroy( midi_note_packet_t **note_packet )
{
	FREENULL( (void **)note_packet );
}

int midi_note_packet_unpack( midi_note_packet_t **midi_note, unsigned char *packet, size_t packet_len )
{
	int ret = 0;


	*midi_note = NULL;

	if( ! packet ) return -1;

	if( packet_len != sizeof( midi_note_packet_t ) )
	{
		fprintf(stderr, "Expecting %d, got %zd\n", sizeof( midi_note_packet_t ), packet_len );
		return -1;
	}

	*midi_note = midi_note_packet_create();
	
	if( ! *midi_note ) return -1;

	(*midi_note)->command = (packet[0] & 0xf0) >> 4 ;
	(*midi_note)->channel = (packet[0] & 0x0f);
	(*midi_note)->note = ( packet[1] & 0x7f );
	(*midi_note)->velocity = ( packet[2] & 0x7f );

	return ret;
}


void midi_note_packet_dump( midi_note_packet_t *note_packet )
{
	if(! note_packet ) return;

	fprintf( stderr, "Note Packet:\n");
	fprintf( stderr, "\tCommand : %02x\n", note_packet->command );
	fprintf( stderr, "\tChannel : %02x\n", note_packet->channel );
	fprintf( stderr, "\tNote    : %02x\n", note_packet->note );
	fprintf( stderr, "\tVelocity: %02x\n", note_packet->velocity );
}
