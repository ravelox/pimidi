/*
   This file is part of raveloxmidi.

   Copyright (C) 2020 Dave Kelly

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

#include "midi_state.h"
#include "midi_command.h"
#include "utils.h"

#include "logging.h"

// Sec 3.2 of RFC6295
// As we note above, the first channel command in the MIDI list MUST
// include a status octet.  However, the corresponding command in the
//original MIDI source data stream might not have a status octet (in
// this case, the source would be coding the command using running
// status)
static unsigned char running_status = 0;

void midi_state_lock( midi_state_t *state )
{
	if( ! state ) return;

	pthread_mutex_lock( &(state->lock) );
}

void midi_state_unlock( midi_state_t *state )
{
	if( ! state ) return;

	pthread_mutex_unlock( &(state->lock) );
}

midi_state_t *midi_state_create( size_t buffer_size )
{
	midi_state_t *new_midi_state = NULL;

	new_midi_state = (midi_state_t *)malloc( sizeof(midi_state_t) );

	if( ! new_midi_state )
	{
		logging_printf( LOGGING_ERROR, "midi_state_create: Insufficient memory to allocate new midi state\n");
		return NULL;
	}

	new_midi_state->status = MIDI_STATE_INIT;
	new_midi_state->hold = dbuffer_create( DBUFFER_DEFAULT_BLOCK_SIZE );
	new_midi_state->ring = ring_buffer_create( buffer_size );
	new_midi_state->running_status = 0;
	pthread_mutex_init( &(new_midi_state->lock) , NULL);

	return new_midi_state;
}

int midi_state_write( midi_state_t *state, char *in_buffer, size_t in_buffer_len )
{
	int ret = 0;

	if( ! state ) return ret;
	if( ! state->ring ) return ret;
	if( ! in_buffer ) return ret;
	if ( in_buffer_len == 0 ) return ret;

	midi_state_lock( state );

	ring_buffer_write( state->ring, in_buffer, in_buffer_len );

	midi_state_unlock( state );
	
	return 1;
}

void midi_state_destroy( midi_state_t **state )
{

	if( ! state ) return;
	if( ! *state ) return;

	midi_state_lock( (*state) );
	if( (*state)->ring )
	{
		ring_buffer_destroy( &( (*state)->ring ) );
		(*state)->ring = NULL;
	}

	if( (*state)->hold )
	{
		dbuffer_destroy( &( (*state)->hold ) );
		(*state)->hold = NULL;
	}

	midi_state_unlock( (*state) );
	pthread_mutex_destroy( &( (*state)->lock ) );

	free( (*state) );
	*state = NULL;
}

int midi_state_compare( midi_state_t *state, const char *compare, size_t compare_len )
{
	if( ! state ) return -1;
	if( ! state->ring ) return -1;

	return ring_buffer_compare( state->ring, compare, compare_len );
}

int midi_state_char_compare( midi_state_t *state, char compare, size_t index )
{
	if(! state ) return -1;
	if( ! state->ring ) return -1;

	return ring_buffer_char_compare( state->ring, compare, index );
}

char *midi_state_drain( midi_state_t *state, size_t *len)
{
	if( ! len ) return NULL;

	*len = 0;
	if( ! state ) return NULL;
	if( ! state->ring ) return NULL;

	return ring_buffer_drain( state->ring, len );
}

void midi_state_advance( midi_state_t *state, size_t steps )
{
	if( ! state ) return;
	if( ! state->ring ) return;
	
	ring_buffer_advance( state->ring, steps );
}

int midi_state_read_byte( midi_state_t *state, uint8_t *byte )
{
	if( ! state )
	{
		*byte = 0;
		return -1;
	}

	if( ! state )
	{
		*byte = 0;
		return -1;
	}

	
}

midi_command_t *midi_state_get_command( midi_state_t *state )
{
	midi_command_t *command = NULL;
	
	if(! state ) goto midi_state_get_command_return;

midi_state_get_command_return:
	return command;
}
