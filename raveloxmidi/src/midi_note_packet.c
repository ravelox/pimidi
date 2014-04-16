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

int midi_note_packet_pack( midi_note_packet_t *midi_note, unsigned char **packet, size_t *packet_len )
{
	int ret = 0;

	*packet = NULL;
	*packet_len = 0;

	if( ! midi_note )
	{
		return -1;
	}

	*packet = (unsigned char *)malloc( PACKED_MIDI_NOTE_PACKET_SIZE );
	
	if( !packet) return -1;

	*packet_len = PACKED_MIDI_NOTE_PACKET_SIZE;

	(*packet)[0] = ( midi_note->command << 4 ) + (midi_note->channel & 0x0f);
	(*packet)[1] = ( midi_note->note & 0x7f );
	(*packet)[2] = ( midi_note->velocity & 0x7f );

	return ret;
}

void midi_note_packet_dump( midi_note_packet_t *note_packet )
{
	if(! note_packet ) return;

	fprintf( stderr, "Note Packet( ");
	fprintf( stderr, "Command=%d , ", note_packet->command );
	fprintf( stderr, "Channel=%d , ", note_packet->channel );
	fprintf( stderr, "Note=%d , ", note_packet->note );
	fprintf( stderr, "Velocity=%d )\n", note_packet->velocity );
}
