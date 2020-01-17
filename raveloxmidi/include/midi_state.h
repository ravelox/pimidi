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

#include <pthread.h>

#include "ring_buffer.h"

typedef struct midi_state_t {
	unsigned int status;
	size_t buffer_len;
	ring_buffer_t *ring;
	size_t waiting_bytes;
	pthread_mutex_t lock;
} midi_state_t;

midi_state_t *midi_state_create( size_t size );
void midi_state_destroy( midi_state_t **state );
void midi_state_lock( midi_state_t *state );
void midi_state_unlock( midi_state_t *state );
int midi_state_write( midi_state_t *state, char *buffer, size_t buffer_len);

#endif
