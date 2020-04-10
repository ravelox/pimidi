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

#ifndef MIDI_COMMAND_H
#define MIDI_COMMAND_H

#include "midi_note.h"

typedef struct channel_message_t {
	unsigned char	channel:4;
	unsigned char	message:4;
} channel_message_t;

typedef struct system_message_t {
	unsigned char message;
} system_message_t;

enum midi_message_type_t {
	MIDI_NULL = 0x00,
	MIDI_NOTE_OFF,
	MIDI_NOTE_ON,
	MIDI_POLY_PRESSURE,
	MIDI_CONTROL_CHANGE,
	MIDI_PROGRAM_CHANGE,
	MIDI_CHANNEL_PRESSURE,
	MIDI_PITCH_BEND,
	MIDI_SYSEX,
	MIDI_TIME_CODE_QF,
	MIDI_SONG_POSITION,
	MIDI_SONG_SELECT,
	MIDI_TUNE_REQUEST,
	MIDI_END_SYSEX,
	MIDI_TIMING_CLOCK,
	MIDI_START,
	MIDI_CONTINUE,
	MIDI_STOP,
	MIDI_ACTIVE_SENSING,
	MIDI_RESET
} midi_message_type_t;

typedef struct midi_message_t {
	unsigned char message;
	enum midi_message_type_t type;
	char *description;
	unsigned char len;
} midi_message_t;

typedef struct midi_command_t {
	uint64_t	delta;
	union {
		channel_message_t	channel_message;
		system_message_t	system_message;
		unsigned char		status;
	};
	size_t data_len;
	unsigned char *data;
} midi_command_t;

midi_command_t *midi_command_create(void);
void midi_command_destroy( void **data );
void midi_command_reset( midi_command_t *command );
void midi_command_map( midi_command_t *command , char **description, enum midi_message_type_t *message_type );
unsigned char midi_command_bytes_needed( unsigned char command );

void midi_command_dump( void *data );
int midi_note_from_command( midi_command_t *command , midi_note_t **midi_note );

#endif
