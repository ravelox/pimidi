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
#include <string.h>
#include <stdint.h>

#include "config.h"

#include "midi_note.h"
#include "midi_command.h"
#include "utils.h"

#include "logging.h"


static char *midi_note_name[] = {
	"C-1","C#-1","D-1","D#-1","E-1","F-1","F#-1","G-1","G#-1","A-1","A#-1","B-1",
	"C0","C#0","D0","D#0","E0","F0","F#0","G0","G#0","A0","A#0","B0",
	"C1","C#1","D1","D#1","E1","F1","F#1","G1","G#1","A1","A#1","B1",
	"C2","C#2","D2","D#2","E2","F2","F#2","G2","G#2","A2","A#2","B2",
	"C3","C#3","D3","D#3","E3","F3","F#3","G3","G#3","A3","A#3","B3",
	"C4","C#4","D4","D#4","E4","F4","F#4","G4","G#4","A4","A#4","B4",
	"C5","C#5","D5","D#5","E5","F5","F#5","G5","G#5","A5","A#5","B5",
	"C6","C#6","D6","D#6","E6","F6","F#6","G6","G#6","A6","A#6","B6",
	"C7","C#7","D7","D#7","E7","F7","F#7","G7","G#7","A7","A#7","B7",
	"C8","C#8","D8","D#8","E8","F8","F#8","G8","G#8","A8","A#8","B8",
	"C9","C#9","D9","D#9","E9","F9","F#9","G9"
};

midi_note_t * midi_note_create( void )
{
	midi_note_t *new_note;

	new_note = (midi_note_t *) X_MALLOC( sizeof( midi_note_t ) );

	if( ! new_note )  return NULL;

	memset( new_note, 0, sizeof( midi_note_t ) );

	return new_note;
}

void midi_note_destroy( midi_note_t **midi_note )
{
	X_FREENULL( "midi_note", (void **)midi_note );
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

	*buffer = (unsigned char *)X_MALLOC( PACKED_MIDI_NOTE_SIZE );
	
	if( !*buffer) return -1;

	*buffer_len = PACKED_MIDI_NOTE_SIZE;

	(*buffer)[0] = ( midi_note->command << 4 ) + (midi_note->channel & 0x0f);
	(*buffer)[1] = ( midi_note->note & 0x7f );
	(*buffer)[2] = ( midi_note->velocity & 0x7f );

	return ret;
}

void midi_note_dump( midi_note_t *midi_note )
{
	INFO_ONLY;
	if(! midi_note ) return;

	logging_printf( LOGGING_INFO, "MIDI Note(cmd=%s, c=%d, n=%s (0x%02x), v=%d)\n", 
		( midi_note->command == 0x9 ? "NoteOn" : "NoteOff" ),
		midi_note->channel + 1,
		( (midi_note->note >= 0) && (midi_note->note <= 127) ? midi_note_name[(unsigned char)(midi_note->note)] : "toobig"), midi_note->note,
		midi_note->velocity);
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

