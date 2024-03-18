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
#include <string.h>
#include <stdint.h>

#include "config.h"

#include "midi_command.h"
#include "utils.h"

#include "logging.h"

static midi_message_t midi_message_map[] = {
        { 0x8, MIDI_NOTE_OFF, "Note Off", 2},
        { 0x9, MIDI_NOTE_ON, "Note On", 2},
        { 0xa, MIDI_POLY_PRESSURE, "Polyphonic Key Pressure (Aftertouch)" , 2},
        { 0xb, MIDI_CONTROL_CHANGE, "Control Change" , 2},
        { 0xc, MIDI_PROGRAM_CHANGE, "Program Change" , 1},
        { 0xd, MIDI_CHANNEL_PRESSURE, "Channel Pressure (Aftertouch)" , 1},
        { 0xe, MIDI_PITCH_BEND, "Pitch Bend Change" ,2 },
        { 0xf0, MIDI_SYSEX, "System Exclusive" , 0},
        { 0xf1, MIDI_TIME_CODE_QF, "Midi Time Code QF" , 1},
        { 0xf2, MIDI_SONG_POSITION, "Song Position" , 2},
        { 0xf3, MIDI_SONG_SELECT, "Song Select" , 1},
        { 0xf4, MIDI_NULL, "Reserved" , 0},
        { 0xf5, MIDI_NULL, "Reserved" ,0},
        { 0xf6, MIDI_TUNE_REQUEST, "Tune Request" , 0},
        { 0xf7, MIDI_END_SYSEX,"End SysEx" ,0},
        { 0xf8, MIDI_TIMING_CLOCK, "Timing Clock" , 0},
        { 0xf9, MIDI_NULL, "Reserved" ,0 },
        { 0xfa, MIDI_START, "Start" ,0},
        { 0xfb, MIDI_CONTINUE, "Continue" ,0},
        { 0xfc, MIDI_STOP, "Stop" ,0},
        { 0xfd, MIDI_NULL, "Reserved" , 0},
        { 0xfe, MIDI_ACTIVE_SENSING, "Active Sensing" , 0},
        { 0xff, MIDI_RESET, "RESET" , 0 },
        { 0x00, MIDI_NULL, "NULL" , 0}
};

midi_command_t *midi_command_create(void)
{
	midi_command_t *new_command = NULL;

	new_command = ( midi_command_t * ) X_MALLOC ( sizeof( midi_command_t ) );

	if( ! new_command ) return NULL;

	new_command->delta = 0;
	new_command->status = 0;
	new_command->data = NULL;

	return new_command;
}

void midi_command_destroy( void **data )
{
	midi_command_t **command = NULL;

	if( ! data ) return;
	if( ! *data ) return;

	command = (midi_command_t **)data;
	if( (*command)->data ) X_FREENULL( "midi_command:command->data",(void **) &( (*command)->data ) );

	X_FREENULL( "midi_command:command", (void **) command );
}

void midi_command_reset( midi_command_t *command )
{
	if( ! command ) return;

	command->delta = 0;
	command->status = 0;

	if(command->data) X_FREE( command->data );
	command->data = NULL;
}

void midi_command_map( midi_command_t *command, char **description, enum midi_message_type_t *message_type)
{
	uint32_t i;
	uint8_t status = 0;

	if( description ) *description = "Unknown";
	*message_type = MIDI_NULL;

	for( i = 0; midi_message_map[i].message != 0x00; i++ )
	{
		if( command->status >= 0xf0 )
		{
			if( midi_message_map[i].message == command->status ) break;
			continue;
		}

		status = ( (command->status & 0xf0) >> 4 );
		if( status == midi_message_map[i].message ) break;
	}

	if( midi_message_map[i].message != 0x00 )
	{
		if( description ) *description = midi_message_map[i].description;
		*message_type = midi_message_map[i].type;
	}
}

unsigned char midi_command_bytes_needed( unsigned char command )
{
	uint32_t i;

	logging_printf( LOGGING_DEBUG, "midi_command_bytes_needed: command=0x%02x\t need_to_normalize=%u\n", command , command < 0xF0 );

	// Normalize the provided command
	if( command < 0xF0 )
	{
		command = ( command & 0xF0 ) >> 4;
	}

	logging_printf( LOGGING_DEBUG, "midi_command_bytes_needed: normalized command=0x%02x\n", command );

	for( i = 0; midi_message_map[i].message != 0x00; i++ )
	{
		if( midi_message_map[i].message == command )
		{
			return midi_message_map[i].len;
		}
	}

	return 0;
}

void midi_command_set( midi_command_t *command, uint64_t delta, uint8_t status, uint8_t *data, size_t data_len )
{
	if( ! command ) return;

	command->delta = delta;
	command->status = status;
	command->data = NULL;
	command->data_len = 0;
	
	if( data )
	{
		command->data = ( unsigned char * ) X_MALLOC( data_len );
		if( ! command->data )
		{
			logging_printf( LOGGING_ERROR, "midi_command_set: Insufficient memory to create command data buffer\n");
		return;
		}

		memset( command->data, 0, data_len );
		memcpy( command->data, data, data_len );
		command->data_len = data_len;
	}
}

void midi_command_dump( void *data )
{
	char *description = NULL;
	enum midi_message_type_t message_type;
	midi_command_t *command = NULL;

	DEBUG_ONLY;

	if( ! data ) return;

	command = (midi_command_t *)data;
	midi_command_map( command, &description, &message_type );

	logging_printf(LOGGING_DEBUG, "MIDI Command: status=0x%02X,description=\"%s\"\n", command->status, description );
	logging_printf(LOGGING_DEBUG, "\tchannel_message:channel=0x%0x message=0x%0x\n", command->channel_message.channel, command->channel_message.message);
	logging_printf(LOGGING_DEBUG, "\tsystem_message: message=0x%0x\n", command->system_message.message); 
	logging_printf(LOGGING_DEBUG, "\tdelta=%zu, data_len=%u\n", command->delta, command->data_len);
}
