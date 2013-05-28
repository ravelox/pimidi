#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include <arpa/inet.h>

#include "midi_note_packet.h"

int midi_note_packet_unpack( midi_note_packet_t **midi_note, unsigned char *packet, size_t packet_len )
{
	int ret = 0;

	if( ! packet ) return -1;

	if( packet_len != sizeof( midi_note_packet_t ) )
	{
		return -1;
	}

	*midi_note = (midi_note_packet_t *) malloc( sizeof( midi_note_packet_t ) );

	if( ! *midi_note ) return -1;

	memcpy( *midi_note, packet, packet_len );

	return ret;
}

void midi_note_packet_destroy( midi_note_packet_t **note_packet )
{
	if( ! note_packet ) return;
	if( ! *note_packet ) return;

	free( *note_packet );
	*note_packet = NULL;
}

void midi_note_packet_dump( midi_note_packet_t *note_packet )
{
	if(! note_packet ) return;

	fprintf( stderr, "Note Packet:\n");
	fprintf( stderr, "\tSig     : %2x\n", note_packet->signature );
	fprintf( stderr, "\tChannel : %2d\n", note_packet->channel );
	fprintf( stderr, "\tNote    : %3d\n", note_packet->note );
	fprintf( stderr, "\tVelocity: %3d\n", note_packet->velocity );
}
