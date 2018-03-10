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

#ifndef CHAPTER_P_JOURNAL_H
#define CHAPTER_P_JOURNAL_H

typedef struct chapter_p_t {
	uint8_t	S;
	uint8_t program;
	uint8_t B;
	uint8_t bank_msb;
	uint8_t	X;
	uint8_t bank_lsb;
} chapter_p_t;
#define CHAPTER_P_PACKED_SIZE	3

void chapter_p_pack( chapter_p_t *chapter_p, unsigned char **packed, size_t *size );
void chapter_p_unpack( unsigned char *packed, size_t size, chapter_p_t **chapter_p );
chapter_p_t *chapter_p_create( void );
void chapter_p_destroy( chapter_p_t **chapter_p );
void chapter_p_dump( chapter_p_t *chapter_p );
void chapter_p_reset( chapter_p_t *chapter_p );

#endif
