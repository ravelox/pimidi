#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include <arpa/inet.h>

#include "midi_note_packet.h"


typedef struct midi_note_packet_t {
	char	signature;
	char	channel;
	char	note;
	char	velocity;
} midi_note_packet_t;

int midi_note_packet_unpack( midi_note_packet_t **midi_note, char *packet, size_t packet_len )
{
	int ret = 0;

	if( ! packet ) return -1;

	if( packet_len != sizeof( midi_note_packet_t ) )
	{
		return -1;
	}

	*midi_note = (midi_note_packet_t *) malloc( sizeof( midi_note_packet_t ) );

	if( ! *midi_note ) return -1;

	memcpy( &(*midi_note), packet, packet_len );

	return ret;
}

void midi_note_packet_destroy( midi_note_packet_t **note_packet )
{
	if( ! note_packet ) return;
	if( ! *note_packet ) return;

	free( *note_packet );
	*note_packet = NULL;
}
