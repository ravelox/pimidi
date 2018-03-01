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

#ifndef CHAPTER_C_JOURNAL_H
#define CHAPTER_C_JOURNAL_H

#ifndef CHAPTER_C
#define CHAPTER_C	0x40
#endif

typedef struct chapterc_header_t {
	unsigned char	S:1;
	unsigned char	len:7;
} chapterc_header_t;
#define CHAPTERC_HEADER_PACKED_SIZE	1

typedef struct alt_controller_t {
	unsigned char 	T:1;
	unsigned char 	alt:6;
} alt_controller_t;

typedef struct controller_log_t {
	unsigned char	S:1;
	unsigned char	num:7;
	unsigned char	A:1;
	union valuealt {
		alt_controller_t	alt_controller;
		unsigned char		value:7;
	} valuealt;
} controller_log_t;

#define CHAPTER_C_PACKED_SIZE 2

#define MAX_CONTROLLER_LOGS	127
typedef struct chapterc_t {
	chapterc_header_t	*header;
	unsigned char		num_controllers;
	controller_log_t	controller_log[ MAX_CONTROLLER_LOGS ];
} chapterc_t;

void chapterc_header_pack( chapterc_header_t *header , unsigned char **packed , size_t *size );
void chapterc_header_destroy( chapterc_header_t **header );
chapterc_header_t * chapterc_header_create( void );
void chapterc_pack( chapterc_t *chapterc, unsigned char **packed, size_t *size );
chapterc_t * chapterc_create( void );
void chapterc_destroy( chapterc_t **chapterc );
void chapterc_header_dump( chapterc_header_t *header );
void chapterc_header_reset( chapterc_header_t *header );
void chapterc_dump( chapterc_t *chapterc );
void chapterc_reset( chapterc_t *chapterc );

#endif
