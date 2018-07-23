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

#ifndef MIDI_CONTROL_H
#define MIDI_CONTROL_H

#include "midi_command.h"

typedef struct midi_control_t {
	unsigned char	channel:4;
	unsigned char	command:4;
	unsigned char	controller_number;
	char		controller_value;
} midi_control_t;

#define MIDI_COMMAND_CONTROL_CHANGE	0x0B
#define PACKED_MIDI_CONTROL_SIZE	3

midi_control_t * midi_control_create( void );
void midi_control_destroy( midi_control_t **midi_control );
int midi_control_unpack( midi_control_t **midi_control, unsigned char *packet, size_t packet_len );
int midi_control_pack( midi_control_t *midi_control, unsigned char **packet, size_t *packet_len );
void midi_control_dump( midi_control_t *midi_control );
int midi_control_from_command( midi_command_t *command , midi_control_t **midi_control );

#endif
