/*
   This file is part of raveloxmidi.

   Copyright (C) 2019 Dave Kelly

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

#ifndef MIDI_STATE_H
#define MIDI_STATE_H

#include <stdint.h>
#include <pthread.h>

#include "data_table.h"
#include "ring_buffer.h"
#include "dbuffer.h"

typedef enum midi_state_status_t {
	MIDI_STATE_INIT,
	MIDI_STATE_WAIT_DELTA,
	MIDI_STATE_WAIT_COMMAND,
	MIDI_STATE_WAIT_BYTE_1,
	MIDI_STATE_WAIT_BYTE_2,
	MIDI_STATE_WAIT_END_SYSEX,
	MIDI_STATE_COMMAND_RECEIVED
} midi_state_status_t;

typedef struct midi_state_t {
	midi_state_status_t status;
	unsigned char running_status;
	ring_buffer_t *ring;
	dbuffer_t *hold;
	pthread_mutex_t lock;
	uint64_t	current_delta;
	uint8_t	partial_sysex;
} midi_state_t;

midi_state_t *midi_state_create( size_t size );
void midi_state_destroy( midi_state_t **state );
void midi_state_lock( midi_state_t *state );
void midi_state_unlock( midi_state_t *state );
int midi_state_write( midi_state_t *state, char *buffer, size_t buffer_len);
int midi_state_compare( midi_state_t *state, const char *compare, size_t compare_len );
int midi_state_char_compare( midi_state_t *state, char compare, size_t index );
char *midi_state_drain( midi_state_t *state, size_t *len);
void midi_state_advance( midi_state_t *state, size_t steps );

void midi_state_reset( midi_state_t *state );

void midi_state_dump( midi_state_t *state );

void midi_state_to_commands( midi_state_t *state, data_table_t **table, char get_delta );

#endif
