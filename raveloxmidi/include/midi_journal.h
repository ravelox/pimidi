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

#define CHAPTER_P	0x80
#define CHAPTER_C	0x40
#define CHAPTER_M	0x20
#define CHAPTER_W	0x10
#define CHAPTER_N	0x08
#define CHAPTER_E	0x04
#define CHAPTER_T	0x02
#define CHAPTER_A	0x01

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

typedef struct channel_t {
	channel_header_t *header;
	chaptern_t *chaptern;
} channel_t;

#define MAX_MIDI_CHANNELS	16

typedef struct journal_t {
	journal_header_t *header;
	channel_t *channels[MAX_MIDI_CHANNELS];
} journal_t;


void journal_header_pack( journal_header_t *header , char **packed , size_t *size );
journal_header_t * journal_header_create( void );
void journal_header_destroy( journal_header_t **header );
void channel_header_pack( channel_header_t *header , char **packed , size_t *size );
void channel_header_destroy( channel_header_t **header );
channel_header_t * channel_header_create( void );
void chaptern_header_pack( chaptern_header_t *header , char **packed , size_t *size );
void chaptern_header_destroy( chaptern_header_t **header );
chaptern_header_t * chaptern_header_create( void );
void midi_note_pack( midi_note_t *note , char **packed , size_t *size );
void midi_note_destroy( midi_note_t **note );
midi_note_t * midi_note_create( void );
void chaptern_pack( chaptern_t *chaptern, char **packed, size_t *size );
chaptern_t * chaptern_create( void );
void chaptern_destroy( chaptern_t **chaptern );
void channel_pack( channel_t *channel, char **packed, size_t *size );
void channel_destroy( channel_t **channel );
channel_t * channel_create( void );
void journal_pack( journal_t *journal, char **packed, size_t *size );
int journal_init( journal_t **journal );
void journal_destroy( journal_t **journal );
void midi_journal_add_note( journal_t *journal, uint32_t seq, char channel, char note, char velocity );
void midi_note_dump( midi_note_t *note );
void chaptern_header_dump( chaptern_header_t *header );
void chaptern_dump( chaptern_t *chaptern );
void channel_header_dump( channel_header_t *header );
void channel_journal_dump( channel_t *channel );
void journal_header_dump( journal_header_t *header );
void journal_dump( journal_t *journal );
int journal_has_data( journal_t *journal );

#endif
