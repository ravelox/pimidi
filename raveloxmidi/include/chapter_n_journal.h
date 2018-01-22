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

#ifndef CHAPTER_N_JOURNAL_H
#define CHAPTER_N_JOURNAL_H

#include "midi_note_packet.h"

#define MAX_CHAPTERN_NOTES	127
#define MAX_OFFBITS		16

typedef struct chaptern_header_t {
	unsigned char	B:1;
	unsigned char	len:7;
	unsigned char	low:4;
	unsigned char	high:4;
} chaptern_header_t;
#define CHAPTERN_HEADER_PACKED_SIZE	2

typedef struct midi_note_t {
	unsigned char	S:1;
	unsigned char	num:7;
	unsigned char	Y:1;
	unsigned char	velocity:7;
} midi_note_t;
#define MIDI_NOTE_PACKED_SIZE	2

typedef struct chaptern_t {
	chaptern_header_t	*header;
	uint16_t		num_notes;
	midi_note_t 		*notes[MAX_CHAPTERN_NOTES];
	char			*offbits;
} chaptern_t;

void chaptern_header_pack( chaptern_header_t *header , unsigned char **packed , size_t *size );
void chaptern_header_destroy( chaptern_header_t **header );
chaptern_header_t * chaptern_header_create( void );
void midi_note_pack( midi_note_t *note , char **packed , size_t *size );
void midi_note_destroy( midi_note_t **note );
midi_note_t * midi_note_create( void );
void chaptern_pack( chaptern_t *chaptern, char **packed, size_t *size );
chaptern_t * chaptern_create( void );
void chaptern_destroy( chaptern_t **chaptern );
void midi_journal_add_note( journal_t *journal, uint32_t seq, midi_note_packet_t *note_packet );
void midi_note_dump( midi_note_t *note );
void midi_note_reset( midi_note_t *note );
void chaptern_header_dump( chaptern_header_t *header );
void chaptern_header_reset( chaptern_header_t *header );
void chaptern_dump( chaptern_t *chaptern );
void chaptern_reset( chaptern_t *chaptern );

#endif
