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

#ifndef MIDI_SENDER_H
#define MIDI_SENDER_H

#include "data_context.h"

typedef struct midi_sender_context_t {
	uint32_t ssrc;
	int	alsa_card_hash;
} midi_sender_context_t;

void midi_sender_init( void );
void midi_sender_start( void );
void midi_sender_stop( void );
void midi_sender_teardown( void );

void midi_sender_send_from_state( midi_state_t *state, void *context);
void midi_sender_send_single( midi_command_t *command, uint32_t originator_ssrc , int originator_device_hash );

void midi_sender_add( void *data, data_context_t *context );

#endif
