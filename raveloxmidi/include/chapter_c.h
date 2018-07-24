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

#define MAX_CHAPTER_C_CONTROLLERS	127

typedef struct controller_log_t {
	uint8_t S;
	uint8_t number;
	uint8_t A;
	uint8_t T;
	uint8_t value;
} controller_log_t;

#define PACKED_CONTROLLER_LOG_SIZE 2

typedef struct chapter_c_t {
	uint8_t S;
	uint8_t len;
	controller_log_t controller_log[MAX_CHAPTER_C_CONTROLLERS];
} chapter_c_t;

#define PACKED_CHAPTER_C_HEADER_SIZE 1

chapter_c_t *chapter_c_create( void );
void chapter_c_unpack( unsigned char *packed, size_t size, chapter_c_t **chapter_c );
void chapter_c_pack( chapter_c_t *chapter_c, unsigned char **packed, size_t *size );
void chapter_c_destroy( chapter_c_t **chapter_c );
void chapter_c_reset( chapter_c_t *chapter_c );
void chapter_c_dump( chapter_c_t *chapter_c );

controller_log_t *controller_log_create( void );
void controller_log_destroy( controller_log_t **controller_log );
void controller_log_reset( controller_log_t *controller_log );
void controller_log_dump( controller_log_t *controller_log );

#endif
