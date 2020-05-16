/*
   This file is part of raveloxmidi.

   Copyright (C) 2020 Dave Kelly

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

#ifndef DBUFFER_H
#define DBUFFER_H

#include <pthread.h>

typedef struct dbuffer_t
{
	size_t len;
	size_t block_size;
	size_t num_blocks;
	unsigned char *data;
	pthread_mutex_t	lock;
} dbuffer_t;

dbuffer_t *dbuffer_create( size_t block_size );
int dbuffer_reset( dbuffer_t *dbuffer );
void dbuffer_destroy( dbuffer_t **dbuffer );
void dbuffer_dump( dbuffer_t *dbuffer );
size_t dbuffer_len( dbuffer_t *dbuffer );
size_t dbuffer_write( dbuffer_t *dbuffer, char *in_buffer, size_t in_buffer_len );
size_t dbuffer_read( dbuffer_t *dbuffer, char **out_buffer );

#define DBUFFER_DEFAULT_BLOCK_SIZE	512

#endif
