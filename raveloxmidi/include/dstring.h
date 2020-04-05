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

#ifndef dstring_H
#define dstring_H

#include <pthread.h>

typedef struct dstring_t
{
	size_t block_size;
	size_t num_blocks;
	unsigned char *data;
	pthread_mutex_t	lock;
} dstring_t;

dstring_t *dstring_create( size_t block_size );
int dstring_reset( dstring_t *dstring );
void dstring_destroy( dstring_t **dstring );
void dstring_dump( dstring_t *dstring );
size_t dstring_len( dstring_t *dstring );
size_t dstring_append( dstring_t *dstring, char *in_string );
unsigned char *dstring_value( dstring_t *dstring );

#define DSTRING_DEFAULT_BLOCK_SIZE	512

#endif
