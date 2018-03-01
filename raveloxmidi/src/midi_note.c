/*
   This file is part of raveloxmidi.

   Copyright (C) 2018 Dave Kelly

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

#include "midi_note.h"
#include "utils.h"

#include "logging.h"

void midi_note_pack( midi_note_t *note , char **packed , size_t *size )
{
	char *p = NULL;

	*packed = NULL;
	*size = 0;

	if( ! note ) return;

	*packed = ( char *)malloc( MIDI_NOTE_PACKED_SIZE );

	if( ! packed ) return;

	memset( *packed, 0, MIDI_NOTE_PACKED_SIZE );
	p = *packed;

	*p |= ( ( note->S & 0x01 ) << 7 );
	*p |= ( note->num & 0x7f ) ;

	p += sizeof( char );
	*size += sizeof( char );

	*p |= ( ( note->Y & 0x01 ) << 7 );
	*p |= ( note->velocity & 0x7f );

	*size += sizeof( char );
}

void midi_note_destroy( midi_note_t **note )
{
	FREENULL( (void **)note );
}

midi_note_t * midi_note_create( void )
{
	midi_note_t *note = NULL;
	
	note = ( midi_note_t * ) malloc( sizeof( midi_note_t ) );

	if( note )
	{
		memset( note , 0 , sizeof( midi_note_t ) );
	}

	return note;
}

void midi_note_dump( midi_note_t *note )
{
	if( ! note ) return;

	logging_printf( LOGGING_DEBUG, "NOTE: S=%d num=%u Y=%d velocity=%u\n", note->S, note->num, note->Y, note->velocity);
}

void midi_note_reset( midi_note_t *note )
{
	if( ! note ) return;

	note->S = 0;
	note->num = 0;
	note->Y = 0;
	note->velocity = 0;
}
