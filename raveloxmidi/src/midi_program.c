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

#include "config.h"

#include "midi_program.h"
#include "midi_command.h"
#include "utils.h"

#include "logging.h"

midi_program_t * midi_program_create( void )
{
	midi_program_t *new_program = NULL;

	new_program = (midi_program_t *) malloc( sizeof( midi_program_t ) );

	if( ! new_program )
	{
		logging_printf(LOGGING_ERROR,"midi_program_create: Unable to allocate memory for new midi_program_t\n");
		return NULL;
	}

	memset( new_program, 0, sizeof( midi_program_t ) );
	return new_program;
}

void midi_program_destroy( midi_program_t **midi_program )
{
	FREENULL( "midi_program", (void **)midi_program );
}

int midi_program_unpack( midi_program_t **midi_program, unsigned char *buffer, size_t buffer_len )
{
	int ret = 0;


	*midi_program = NULL;

	if( ! buffer ) return -1;

	if( buffer_len < PACKED_MIDI_PROGRAM_SIZE)
	{
		logging_printf( LOGGING_DEBUG, "midi_program_unpack: Expecting %d, got %zd\n", PACKED_MIDI_PROGRAM_SIZE, buffer_len );
		return -1;
	}

	*midi_program = midi_program_create();
	
	if( ! *midi_program ) return -1;

	(*midi_program)->command = (buffer[0] & 0xf0) >> 4 ;
	(*midi_program)->channel = (buffer[0] & 0x0f);
	(*midi_program)->B = (buffer[1] & 0x80 ) >> 7;
	(*midi_program)->bank_msb = ( buffer[1] & 0x7f );
	(*midi_program)->X = (buffer[2] & 0x80 ) >> 7;
	(*midi_program)->bank_lsb = ( buffer[2] & 0x7f );

	return ret;
}

int midi_program_pack( midi_program_t *midi_program, unsigned char **buffer, size_t *buffer_len )
{
	int ret = 0;

	*buffer = NULL;
	*buffer_len = 0;

	if( ! midi_program )
	{
		return -1;
	}

	*buffer = (unsigned char *)malloc( PACKED_MIDI_PROGRAM_SIZE );
	
	if( !*buffer) return -1;

	*buffer_len = PACKED_MIDI_PROGRAM_SIZE;

	(*buffer)[0] = ( midi_program->command << 4 ) + (midi_program->channel & 0x0f);
	(*buffer)[1] = ( midi_program->B << 7) + ( midi_program->bank_msb & 0x7f );
	(*buffer)[2] = ( midi_program->X << 7) + ( midi_program->bank_lsb & 0x7f );

	return ret;
}

void midi_program_dump( midi_program_t *midi_program )
{
	DEBUG_ONLY;
	if(! midi_program ) return;

	logging_printf( LOGGING_DEBUG, "MIDI Program Change: command=0x%02x, channel=0x%02x, S=%d program=%d B=%d, bank_msb=0x%02x, X=%d, bank_lsb=0x%02x\n",
		midi_program->command, midi_program->channel, midi_program->S, midi_program->program, midi_program->B, midi_program->bank_msb, midi_program->X, midi_program->bank_lsb );
}

int midi_program_from_command( midi_command_t *command , midi_program_t **midi_program )
{
	int ret = 0;

	*midi_program = NULL;

	if( ! command ) return -1;

	*midi_program = midi_program_create();
	
	if( ! *midi_program ) return -1;

	(*midi_program)->command = command->channel_message.message;
	(*midi_program)->channel = command->channel_message.channel;
	if( command->data_len > 0 )
	{
		(*midi_program)->program = (command->data[0] & 0x7f);
	}

	return ret;
}

