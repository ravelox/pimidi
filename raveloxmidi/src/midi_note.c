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

#include "config.h"

#include "midi_note.h"
#include "midi_command.h"
#include "utils.h"

#include "logging.h"

midi_note_t * midi_note_create( void )
{
	midi_note_t *new_note;

	new_note = (midi_note_t *) malloc( sizeof( midi_note_t ) );

	if( ! new_note )  return NULL;

	memset( new_note, 0, sizeof( midi_note_t ) );

	return new_note;
}

void midi_note_destroy( midi_note_t **midi_note )
{
	FREENULL( "midi_note", (void **)midi_note );
}

int midi_note_unpack( midi_note_t **midi_note, unsigned char *buffer, size_t buffer_len )
{
	int ret = 0;


	*midi_note = NULL;

	if( ! buffer ) return -1;

	if( buffer_len != sizeof( midi_note_t ) )
	{
		logging_printf( LOGGING_DEBUG, "midi_note_unpack: Expecting %d, got %zd\n", sizeof( midi_note_t ), buffer_len );
		return -1;
	}

	*midi_note = midi_note_create();
	
	if( ! *midi_note ) return -1;

	(*midi_note)->command = (buffer[0] & 0xf0) >> 4 ;
	(*midi_note)->channel = (buffer[0] & 0x0f);
	(*midi_note)->note = ( buffer[1] & 0x7f );
	(*midi_note)->velocity = ( buffer[2] & 0x7f );

	return ret;
}

int midi_note_pack( midi_note_t *midi_note, unsigned char **buffer, size_t *buffer_len )
{
	int ret = 0;

	*buffer = NULL;
	*buffer_len = 0;

	if( ! midi_note )
	{
		return -1;
	}

	*buffer = (unsigned char *)malloc( PACKED_MIDI_NOTE_SIZE );
	
	if( !*buffer) return -1;

	*buffer_len = PACKED_MIDI_NOTE_SIZE;

	(*buffer)[0] = ( midi_note->command << 4 ) + (midi_note->channel & 0x0f);
	(*buffer)[1] = ( midi_note->note & 0x7f );
	(*buffer)[2] = ( midi_note->velocity & 0x7f );

	return ret;
}

void midi_note_dump( midi_note_t *midi_note )
{
	DEBUG_ONLY;
	if(! midi_note ) return;

	logging_printf( LOGGING_DEBUG, "MIDI Note(command=%d,channel=%d,note=%d,velocity=%d)\n",
		midi_note->command, midi_note->channel, midi_note->note, midi_note->velocity);
}

int midi_note_from_command( midi_command_t *command , midi_note_t **midi_note )
{
	int ret = 0;

	if( ! command ) return -1;

	*midi_note = midi_note_create();
	
	if( ! *midi_note ) return -1;

	(*midi_note)->command = command->channel_message.message;
	(*midi_note)->channel = command->channel_message.channel;
	if( command->data_len > 0 )
	{
		(*midi_note)->note = (command->data[0] & 0x7f );
		(*midi_note)->velocity = ( command->data[1] & 0x7f );
	}

	return ret;
}

