/*
   This file is part of raveloxmidi.

   Copyright (C) 2019 Dave Kelly

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

#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <pthread.h>

typedef struct ring_buffer_t
{
	size_t start,end;
	size_t	size;
	size_t used;
	unsigned char *data;
	pthread_mutex_t	lock;
} ring_buffer_t;

ring_buffer_t *ring_buffer_create( size_t size );
void ring_buffer_destroy( ring_buffer_t **ring );
void ring_buffer_reset( ring_buffer_t *ring , size_t ring_buffer_size);

size_t ring_buffer_get_size( ring_buffer_t *ring );

size_t ring_buffer_write( ring_buffer_t *ring, char *data, size_t len );
char *ring_buffer_read( ring_buffer_t *ring, size_t len , int advance);
char ring_buffer_read_byte( ring_buffer_t *ring, uint8_t *byte, int advance );
int ring_buffer_resize( ring_buffer_t *ring, size_t new_size );
int ring_buffer_available( ring_buffer_t *ring, size_t requested );
char *ring_buffer_drain( ring_buffer_t *ring , size_t *len );

void ring_buffer_lock( ring_buffer_t *ring);
void ring_buffer_unlock( ring_buffer_t *ring);

void ring_buffer_dump( ring_buffer_t *ring );

int ring_buffer_compare( ring_buffer_t *ring, const char *compare , size_t compare_len);
int ring_buffer_char_compare( ring_buffer_t *ring, char compare, size_t index );

void ring_buffer_advance( ring_buffer_t *ring, size_t steps );

#define RING_NO		0
#define RING_YES	1
#endif
