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

#ifndef MIDI_JOURNAL_H
#define MIDI_JOURNAL_H

#include "midi_note.h"
#include "midi_control.h"

#include "chapter_p.h"
#include "chapter_n.h"
#include "chapter_c.h"

typedef struct journal_header_t {
	uint8_t	bitfield; // SYAH
	uint8_t totchan;
	uint16_t	seq;
} journal_header_t;
#define JOURNAL_HEADER_PACKED_SIZE	3

typedef struct channel_header_t {
	unsigned char	S:1;
	unsigned char	chan:4;
	unsigned char	H:1;
	unsigned int	len:10;
	uint8_t		bitfield; // PCMWNETA
} channel_header_t;
#define CHANNEL_HEADER_PACKED_SIZE	3

//Bitfield flag positions for each chapter
#define CHAPTER_P	0x80
#define CHAPTER_C	0x40
#define CHAPTER_M	0x20
#define CHAPTER_W	0x10
#define CHAPTER_N	0x08
#define CHAPTER_E	0x04
#define CHAPTER_T	0x02
#define CHAPTER_A	0x01

typedef struct channel_t {
	channel_header_t *header;
	chapter_p_t *chapter_p;
	chapter_n_t *chapter_n;
	chapter_c_t *chapter_c;
} channel_t;

#define MAX_MIDI_CHANNELS	16

typedef struct journal_t {
	journal_header_t *header;
	channel_t *channels[MAX_MIDI_CHANNELS];
} journal_t;

void channel_header_pack( channel_header_t *header , unsigned char **packed , size_t *size );
void channel_header_destroy( channel_header_t **header );
channel_header_t * channel_header_create( void );
void channel_pack( channel_t *channel, char **packed, size_t *size );
void channel_destroy( channel_t **channel );
channel_t * channel_create( void );

void journal_header_pack( journal_header_t *header , char **packed , size_t *size );
journal_header_t * journal_header_create( void );
void journal_header_destroy( journal_header_t **header );
void journal_pack( journal_t *journal, char **packed, size_t *size );
int journal_init( journal_t **journal );
void journal_destroy( journal_t **journal );
void channel_header_dump( channel_header_t *header );
void channel_header_reset( channel_header_t *header );
void channel_journal_dump( channel_t *channel );
void channel_journal_reset( channel_t *channel );
int journal_has_data( journal_t *journal );
void journal_header_dump( journal_header_t *header );
void journal_header_reset( journal_header_t *header );
void journal_dump( journal_t *journal );
void journal_reset( journal_t *journal );

void midi_journal_add_note( journal_t *journal, uint32_t seq, midi_note_t *midi_note );
void midi_journal_add_control( journal_t *journal, uint32_t seq, midi_control_t *midi_control );

#endif
