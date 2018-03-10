/*
   This file is part of raveloxmidi.

   Copyright (C) 2014 Dave Kelly

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

#ifndef MIDI_NOTE_H
#define MIDI_NOTE_H

typedef struct midi_note_t {
	unsigned char	channel:4;
	unsigned char	command:4;
	char		note;
	char		velocity;
} midi_note_t;

#define MIDI_COMMAND_NOTE_ON	0x09
#define MIDI_COMMAND_NOTE_OFF	0x08

#define PACKED_MIDI_NOTE_SIZE	3

midi_note_t * midi_note_create( void );
void midi_note_destroy( midi_note_t **midi_note );
int midi_note_unpack( midi_note_t **midi_note, unsigned char *packet, size_t packet_len );
int midi_note_pack( midi_note_t *midi_note, unsigned char **packet, size_t *packet_len );
void midi_note_dump( midi_note_t *midi_note );

#endif
