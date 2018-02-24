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

#include "midi_command.h"
#include "utils.h"

#include "logging.h"

static midi_message_t midi_message_map[] = {
        { 0x80, MIDI_NOTE_OFF, "Note Off"},
        { 0x90, MIDI_NOTE_ON, "Note On"},
        { 0xa0, MIDI_POLY_PRESSURE, "Polyphonic Key Pressure (Aftertouch)" },
        { 0xb0, MIDI_CONTROL_CHANGE, "Control Change" },
        { 0xc0, MIDI_PROGRAM_CHANGE, "Program Change" },
        { 0xd0, MIDI_CHANNEL_PRESSURE, "Channel Pressure (Aftertouch)" },
        { 0xe0, MIDI_PITCH_BEND, "Pitch Bend Change" },
        { 0xf0, MIDI_SYSEX, "System Exclusive" },
        { 0xf1, MIDI_TIME_CODE_QF, "Midi Time Code QF" },
        { 0xf2, MIDI_SONG_POSITION, "Song Position" },
        { 0xf3, MIDI_SONG_SELECT, "Song Select" },
        { 0xf4, MIDI_NULL, "Reserved" },
        { 0xf5, MIDI_NULL, "Reserved" },
        { 0xf6, MIDI_TUNE_REQUEST, "Tune Request" },
        { 0xf7, MIDI_END_SYSEX,"End SysEx" },
        { 0xf8, MIDI_TIMING_CLOCK, "Timing Clock" },
        { 0xf9, MIDI_NULL, "Reserved" },
        { 0xfa, MIDI_START, "Start" },
        { 0xfb, MIDI_CONTINUE, "Continue" },
        { 0xfc, MIDI_STOP, "Stop" },
        { 0xfd, MIDI_NULL, "Reserved" },
        { 0xfe, MIDI_ACTIVE_SENSING, "Active Sensing" },
        { 0xff, MIDI_RESET, "RESET" },
        { 0x00, MIDI_NULL, "NULL" }
};

midi_command_t *midi_command_create(void)
{
	midi_command_t *new_command = NULL;

	new_command = ( midi_command_t * ) malloc ( sizeof( midi_command_t ) );

	if( ! new_command ) return NULL;

	midi_command_reset( new_command );

	return new_command;
}

void midi_command_destroy( midi_command_t **command )
{
	if( ! command ) return;
	if( ! *command ) return;

	if( (*command)->data ) FREENULL( (void **) &( (*command)->data ) );

	FREENULL( (void **) command );
}

void midi_command_reset( midi_command_t *command )
{
	if( ! command ) return;

	command->delta = 0;
	command->status = 0;

	if(command->data) free( command->data );
	command->data = NULL;
}

void midi_command_map( midi_command_t *command, char **description, enum midi_message_type_t *message_type)
{
	uint32_t i;

	*description = "Unknown";
	*message_type = MIDI_NULL;

	for( i = 0; midi_message_map[i].message != 0x00; i++ )
	{
		logging_printf(LOGGING_DEBUG, "MIDI Command Map: status_0f=%u,match=%u,map_message=0x%02X,command_status=0x%02X\n", command->status && 0xf0, midi_message_map[i].message == command->status, midi_message_map[i].message, command->status);
		if( command->status & 0xf0 )
		{
			if( midi_message_map[i].message == command->status ) break;
			continue;
		}

		if( command->status & midi_message_map[i].message ) break;
	}

	if( midi_message_map[i].message != 0x00 )
	{
		*description = midi_message_map[i].description;
		*message_type = midi_message_map[i].type;
	}
}

void midi_command_dump( midi_command_t *command )
{
	char *description = NULL;
	enum midi_message_type_t message_type;

	if( ! command ) return;

	midi_command_map( command, &description, &message_type );
	logging_printf(LOGGING_DEBUG, "MIDI Command: status=0x%02X,description=\"%s\"\n", command->status, description );
}


