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

// #include "chapter_n_note_packet.h"

typedef struct chapter_n_note_t {
	unsigned char	S:1;
	unsigned char	num:7;
	unsigned char	Y:1;
	unsigned char	velocity:7;
} chapter_n_note_t;
#define CHAPTER_N_NOTE_PACKED_SIZE	2

#define MAX_CHAPTER_N_NOTES	127
#define MAX_OFFBITS		16

typedef struct chapter_n_header_t {
	unsigned char	B:1;
	unsigned char	len:7;
	unsigned char	low:4;
	unsigned char	high:4;
} chapter_n_header_t;
#define CHAPTER_N_HEADER_PACKED_SIZE	2

typedef struct chapter_n_t {
	chapter_n_header_t	*header;
	uint16_t		num_notes;
	chapter_n_note_t	*notes[MAX_CHAPTER_N_NOTES];
	char			*offbits;
} chapter_n_t;

void chapter_n_header_pack( chapter_n_header_t *header , unsigned char **packed , size_t *size );
void chapter_n_header_destroy( chapter_n_header_t **header );
chapter_n_header_t * chapter_n_header_create( void );
void chapter_n_pack( chapter_n_t *chapter_n, char **packed, size_t *size );
chapter_n_t * chapter_n_create( void );
void chapter_n_destroy( chapter_n_t **chapter_n );
void chapter_n_header_dump( chapter_n_header_t *header );
void chapter_n_header_reset( chapter_n_header_t *header );
void chapter_n_dump( chapter_n_t *chapter_n );
void chapter_n_reset( chapter_n_t *chapter_n );

void chapter_n_note_pack( chapter_n_note_t *note , char **packed , size_t *size );
void chapter_n_note_destroy( chapter_n_note_t **note );
chapter_n_note_t * chapter_n_note_create( void );
void chapter_n_note_dump( chapter_n_note_t *note );
void chapter_n_note_reset( chapter_n_note_t *note );

#endif
