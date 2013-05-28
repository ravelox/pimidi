#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include <arpa/inet.h>

#include "midi_recv.h"

int midi_note_unpack( midi_note_packet_t **midi_note, char *packet, size_t packet_len )
{
	int ret = 0;

	if( ! packet ) return -1;

	if( packet_len < sizeof( midi_note_packet_t ) )
	{
		return -1;
	}

	return ret;
}

