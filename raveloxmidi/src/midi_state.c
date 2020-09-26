/*
   This file is part of raveloxmidi

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
#include "data_table.h"
#include "utils.h"

#include "midi_sender.h"
#include "data_context.h"

#include "logging.h"

void midi_state_lock( midi_state_t *state )
{
	if( ! state ) return;

	X_MUTEX_LOCK( &(state->lock) );
}

void midi_state_unlock( midi_state_t *state )
{
	if( ! state ) return;

	X_MUTEX_UNLOCK( &(state->lock) );
}

midi_state_t *midi_state_create( size_t buffer_size )
{
	midi_state_t *new_midi_state = NULL;

	new_midi_state = (midi_state_t *)X_MALLOC( sizeof(midi_state_t) );

	if( ! new_midi_state )
	{
		logging_printf( LOGGING_ERROR, "midi_state_create: Insufficient memory to allocate new midi state\n");
		return NULL;
	}

	new_midi_state->status = MIDI_STATE_INIT;
	new_midi_state->hold = dbuffer_create( DBUFFER_DEFAULT_BLOCK_SIZE );
	new_midi_state->ring = ring_buffer_create( buffer_size );
	new_midi_state->running_status = 0;
	new_midi_state->partial_sysex = 0;
	pthread_mutex_init( &(new_midi_state->lock) , NULL);

	return new_midi_state;
}

void midi_state_reset( midi_state_t *state )
{
	if( ! state ) return;

	midi_state_lock( state );

	state->status = MIDI_STATE_INIT;
	state->running_status = 0;

	if( state->hold )
	{
		dbuffer_reset( state->hold );
	}

	if( state->ring )
	{
		size_t ring_buffer_size = ring_buffer_get_size( state->ring );
		ring_buffer_reset( state->ring , ring_buffer_size );
	}

	midi_state_unlock( state );
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

	X_FREE( (*state) );
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
	char *return_buffer = NULL;

	if( ! state ) return NULL;
	if( ! len ) return NULL;

	midi_state_lock( state );

	*len = 0;
	if( ! state->ring ) goto midi_state_drain_end;

	return_buffer = ring_buffer_drain( state->ring, len );

midi_state_drain_end:
	midi_state_unlock( state );

	return return_buffer;
}

void midi_state_advance( midi_state_t *state, size_t steps )
{
	if( ! state ) return;

	midi_state_lock( state );
	if( ! state->ring ) goto midi_state_advance_end;
	
	ring_buffer_advance( state->ring, steps );

midi_state_advance_end:
	midi_state_unlock( state );
}

char midi_state_read_byte( midi_state_t *state, uint8_t *byte )
{
	if( ! byte )
	{
		return -1;
	}

	if( ! state )
	{
		*byte = 0;
		return -1;
	}

	if( ! state->ring )
	{
		*byte = 0;
		return -1;
	}

	return ring_buffer_read_byte( state->ring, byte, RING_YES );
}

const char *midi_status_to_string( midi_state_status_t status )
{
	switch( status )
	{
		case MIDI_STATE_INIT: return "MIDI_STATE_INIT";
		case MIDI_STATE_WAIT_DELTA: return "MIDI_STATE_WAIT_DELTA";
		case MIDI_STATE_WAIT_COMMAND: return "MIDI_STATE_WAIT_COMMAND";
		case MIDI_STATE_WAIT_BYTE_1: return "MIDI_STATE_WAIT_BYTE_1";
		case MIDI_STATE_WAIT_BYTE_2: return "MIDI_STATE_WAIT_BYTE_2";
		case MIDI_STATE_WAIT_END_SYSEX: return "MIDI_STATE_WAIT_END_SYSEX";
		case MIDI_STATE_COMMAND_RECEIVED: return "MIDI_STATE_COMMAND_RECEIVED";
		default: return "Unknown";
	}
	return "Unknown";
}

void midi_state_dump( midi_state_t *state )
{
	if( ! state ) return;

	DEBUG_ONLY;

	logging_printf( LOGGING_DEBUG, "midi_state=%p, status=[%s], running_status=0x%02x\n", 
		state, midi_status_to_string( state->status ), state->running_status);

	ring_buffer_dump( state->ring );
	dbuffer_dump( state->hold );
}

void midi_state_send( midi_state_t *state , data_context_t *context, char mode, char z_flag)
{
	uint8_t byte = 0;
	unsigned char bytes_needed = 0;
	char *buffer_value = NULL;
	size_t buffer_len = 0;
	midi_command_t *new_command = NULL;
	char read_status = 0;
	char get_delta = 0;

	if( ! state ) return;

	get_delta = z_flag;

	while( 1 )
	{
		read_status = midi_state_read_byte( state, &byte );

		// If no bytes are available...return
		if( read_status != 0 )
		{
			// Special case if this is a RTP buffer, we need to set the state back to MIDI_STATE_INIT
			if( mode == MIDI_PARSE_MODE_RTP )
			{
				state->status = MIDI_STATE_INIT;
			}
			return;
		}

		logging_printf( LOGGING_DEBUG, "midi_state_send: STATE_IN read_byte byte=0x%02x, running_status=0x%02x, status=%s\n", byte , state->running_status, midi_status_to_string( state->status) );

		// If the delta has been provided, we need to get it first
		if( state->status == MIDI_STATE_INIT)
		{
				state->status = ( get_delta ? MIDI_STATE_WAIT_DELTA : MIDI_STATE_WAIT_COMMAND );
				state->current_delta = 0;
		}

		if( state->status == MIDI_STATE_WAIT_DELTA )
		{
			state->current_delta <<= 8;
			state->current_delta += ( byte & 0x7f );

			// RFC6296 sec 3.1
			// The final octet of the delta time will NOT have bit 7 set
			if( ! (byte & 0x80) )
			{
				state->status = MIDI_STATE_WAIT_COMMAND;
				
			}
			logging_printf( LOGGING_DEBUG, "midi_state_send: delta=%lu\n", state->current_delta);
			continue;
		// Check for real-time MIDI messages that should go through at any time
		} else {
			if( byte >= MIDI_TIMING_CLOCK )
			{
				logging_printf( LOGGING_DEBUG, "midi_state_send: real-time message: message=0x%02x\n", byte);
				new_command = midi_command_create();
				if( ! new_command )
				{
					logging_printf( LOGGING_DEBUG, "midi_state_send: real-time: Insufficient memory to create command structure\n");
					continue;
				}
				midi_command_set( new_command, state->current_delta, byte, NULL, 0);
				midi_sender_add( new_command, context );
				continue;
			}
		}

		if( state->status == MIDI_STATE_WAIT_COMMAND )
		{
			if( byte < 0x80 )
			{
				// If this is a data byte but there is no running status, we need to loop
				if( state->running_status == 0 ) continue;
				
				// Using the running status, determine how many bytes we need
				bytes_needed = midi_command_bytes_needed( state->running_status );
				logging_printf( LOGGING_DEBUG, "midi_state_send: data byte received: need=%d\n", bytes_needed );
				
				// Copy the running status into the hold buffer
				dbuffer_write( state->hold, &(state->running_status), 1);

				// Copy the byte into the hold buffer
				dbuffer_write( state->hold, &byte, 1 );

				switch( bytes_needed )
				{
					case 1:
						state->status = MIDI_STATE_COMMAND_RECEIVED;
						break;
					case 2:
						state->status = MIDI_STATE_WAIT_BYTE_1;
						break;
				}
			} else {
				// This is a command byte
				// Copy command byte to hold buffer
				dbuffer_write( state->hold, &byte, 1);

				// Special cases for SysEx
				if( (byte == 0xF0) || ( state->partial_sysex && (byte==0xF7) ) )
				{
					if( (byte == 0xF0) && (state->partial_sysex == 1) )
					{
						logging_printf( LOGGING_DEBUG, "midi_state_send: SYSEX 0xF0 read: Expected 0xF7\n");
					}
					if( state->partial_sysex == 1 )
					{
						logging_printf( LOGGING_DEBUG, "midi_state_send: partial SysEx\n");
					}
					state->status = MIDI_STATE_WAIT_END_SYSEX;
					state->running_status = 0;
				// RFC6295 - p 19 - Unpaired 0xF7 cancels running status
				} else if( byte == MIDI_END_SYSEX ) {
					state->status = MIDI_STATE_COMMAND_RECEIVED;
					state->running_status = 0;
				} else {
					bytes_needed = midi_command_bytes_needed( byte & 0xF0 );
					logging_printf( LOGGING_DEBUG, "midi_state_send: command byte received: need=%d\n", bytes_needed );

					switch( bytes_needed )
					{
						case 1:
							state->status = MIDI_STATE_WAIT_BYTE_1;
							break;
						case 2:
							state->status = MIDI_STATE_WAIT_BYTE_2;
							break;
						case 0:
						default:
							state->status = MIDI_STATE_COMMAND_RECEIVED;
							break;
					}

					// Set the running status
					state->running_status = byte;
				}
			}
			goto midi_state_send_end;
		}
		
		if( state->status == MIDI_STATE_WAIT_BYTE_2)
		{
			// Copy byte to hold buffer
			dbuffer_write( state->hold, &byte, 1 );

			state->status = MIDI_STATE_WAIT_BYTE_1;
			goto midi_state_send_end;
		}

		if( state->status == MIDI_STATE_WAIT_BYTE_1 )
		{
			// Copy byte to hold buffer
			dbuffer_write( state->hold, &byte, 1 );

			state->status = MIDI_STATE_COMMAND_RECEIVED;
			goto midi_state_send_end;
		}

		if( state->status == MIDI_STATE_WAIT_END_SYSEX )
		{
			// Copy byte to hold buffer
			dbuffer_write( state->hold, &byte, 1 );

			if( (byte==0xF7) || ((state->partial_sysex==1) && (byte==0xF4)) )
			{
				state->status = MIDI_STATE_COMMAND_RECEIVED;
				state->partial_sysex = 0;
			}

			if( byte == 0xF0 )
			{
				state->status = MIDI_STATE_COMMAND_RECEIVED;
				state->partial_sysex = 1;
			}

			// Clear the running status
			state->running_status = 0;

			goto midi_state_send_end;
		}

midi_state_send_end:
		logging_printf( LOGGING_DEBUG, "midi_state_send: STATE_OUT read_byte byte=0x%02x, running_status=0x%02x, status=%s\n", byte , state->running_status, midi_status_to_string( state->status) );

		// If we're waiting for more data, loop around
		if( state->status != MIDI_STATE_COMMAND_RECEIVED) continue;

		// Take the full command from the hold buffer
		buffer_len = dbuffer_read( state->hold, &buffer_value );
		logging_printf( LOGGING_DEBUG, "midi_state_send: hold buffer len=%ld\n", buffer_len );

		dbuffer_reset( state->hold );

		// Create a new midi command structure
		new_command = midi_command_create();

		if( ! new_command )
		{
			logging_printf( LOGGING_ERROR, "midi_state_send: Insufficient memory to create new command\n");
		} else {

			midi_command_set( new_command, state->current_delta, buffer_value[0], buffer_value + 1, buffer_len - 1);

			// Add it to the command sender queue
			midi_sender_add( new_command, context );
		}

		X_FREE( buffer_value );

		// Set the status back to INIT
		state->status = MIDI_STATE_INIT;

		// If this is a RTP buffer, we need to get subsequent deltas
		if( mode == MIDI_PARSE_MODE_RTP ) get_delta = 1;
	}
}
