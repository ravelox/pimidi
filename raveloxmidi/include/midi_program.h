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

#ifndef MIDI_PROGRAM_H
#define MIDI_PROGRAM_H

#include "midi_command.h"

typedef struct midi_program_t {
	unsigned char	channel:4;
	unsigned char	command:4;
	unsigned char	S:1;
	unsigned char	B:1;
	unsigned char	X:1;
	unsigned char	program;
	unsigned char	bank_msb;
	unsigned char	bank_lsb;
} midi_program_t;

#define MIDI_COMMAND_PROGRAM_CHANGE	0x0C
#define PACKED_MIDI_PROGRAM_SIZE	3

midi_program_t * midi_program_create( void );
void midi_program_destroy( midi_program_t **midi_program );
int midi_program_unpack( midi_program_t **midi_program, unsigned char *packet, size_t packet_len );
int midi_program_pack( midi_program_t *midi_program, unsigned char **packet, size_t *packet_len );
void midi_program_dump( midi_program_t *midi_program );
int midi_program_from_command( midi_command_t *command , midi_program_t **midi_program );

#endif
