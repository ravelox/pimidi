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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "utils.h"
#include "logging.h"
#include "ring_buffer.h"
#include "dbuffer.h"

void ring_buffer_lock( ring_buffer_t *ring )
{
	if( ! ring ) return;
	X_MUTEX_LOCK( &(ring->lock) );
}

void ring_buffer_unlock( ring_buffer_t *ring )
{
	if( ! ring ) return;
	X_MUTEX_UNLOCK( &(ring->lock) );
}

size_t ring_buffer_used( ring_buffer_t *ring )
{
	size_t ret = 0;
	if(! ring ) return 0;
	if(! ring->data ) return 0;

	ring_buffer_lock( ring );
	ret = ring->used;
	ring_buffer_unlock( ring );
	return ret;
}

void ring_buffer_dump( ring_buffer_t *ring )
{
	DEBUG_ONLY;

	if( ! ring ) return;

	ring_buffer_lock( ring );

	logging_printf( LOGGING_DEBUG, "ring_buffer=%p,data=%p,size=%zu, start=%zu, end=%zu, used=%zu\n", ring, ring->data,ring->size, ring->start, ring->end, ring->used);

	ring_buffer_unlock( ring );
}

void ring_buffer_reset( ring_buffer_t *ring , size_t size)
{
	if( ! ring ) return;

	ring_buffer_lock( ring );

	ring->size = size;
	ring->start = 0;
	ring->end = 0;
	ring->used = 0;

	if( ring->data )
	{
		X_FREE( ring->data );
		ring->data = ( unsigned char * ) X_MALLOC( size );
		if( ring->data )
		{
			memset( ring->data, 0, size );
		}
	}
	
	ring_buffer_unlock( ring );
}

ring_buffer_t *ring_buffer_create( size_t size )
{
	ring_buffer_t *new_buffer;

	if( size <= 0 )
	{
		logging_printf( LOGGING_ERROR, "ring_buffer_create: invalid size specified: %d\n", size );
		return NULL;
	}

	new_buffer = (ring_buffer_t *)X_MALLOC( sizeof( ring_buffer_t ) );

	if( ! new_buffer )
	{
		logging_printf( LOGGING_ERROR, "ring_buffer_create: insufficient memory to create new buffer\n");
		return NULL;
	}

	new_buffer->data = ( unsigned char * ) X_MALLOC( size );
	if( ! new_buffer->data )
	{
		logging_printf( LOGGING_ERROR, "ring_buffer_create: insufficient memory to create internal data buffer\n");
		X_FREE( new_buffer );
		return NULL;
	}

	pthread_mutex_init( &new_buffer->lock , NULL );
	ring_buffer_reset( new_buffer , size );

	return new_buffer;
}

void ring_buffer_destroy( ring_buffer_t **ring )
{
	if( ! ring ) return;
	if( ! *ring ) return;

	ring_buffer_lock((*ring));
	(*ring)->size = 0;
	(*ring)->start = 0;
	(*ring)->end = 0;
	
	if( (*ring)->data )
	{
		X_FREE( (*ring)->data );
		(*ring)->data = NULL;
	}

	ring_buffer_unlock((*ring));

	pthread_mutex_destroy( &(*ring)->lock );
	
	X_FREENULL("ring", (void **)ring);
}

size_t ring_buffer_write( ring_buffer_t *ring, unsigned char *data, size_t len )
{
	size_t return_val = 0;

	if( !ring ) return 0;
	if( !data ) return 0;
	if( len == 0 ) return 0;

	ring_buffer_lock( ring );

	logging_printf( LOGGING_DEBUG, "ring_buffer_write: stage=start len=%zu, ring_buffer=%p,data=%p,size=%zu, start=%zu, end=%zu, used=%zu\n", len, ring, ring->data,ring->size, ring->start, ring->end, ring->used);

	/* Not enough space */
	if( ring->used + len > ring->size )
	{
		logging_printf( LOGGING_ERROR, "ring_buffer_write: ring=%p Insufficient space available in buffer\n", ring);
		return_val = 0;
		goto ring_buffer_write_end;
	}

	/* There are a number of possible cases */
	return_val = len;
	if( ring->start <= ring->end )
	{
		/* 1: The buffer isn't wrapped and data can be stored contiguously */
		if( ( ring->end + len ) <= ring->size )
		{
			logging_printf( LOGGING_DEBUG, "ring_buffer_write: buffer=contiguous write=contiguous ring=%p\n", ring);
			unsigned char *dest = ring->data + ring->end;

			/* Copy the data */
			memcpy( dest, data, len );

			/* Update the pointers */
			ring->used += len;
			ring->end += len;
		} else {
		/* 2: The buffer isn't wrapped but the data has to be split */
			size_t first_part = ring->size - ring->end;
			size_t second_part = len - first_part;
			unsigned char *dest = NULL;

			logging_printf( LOGGING_DEBUG, "ring_buffer_write: buffer=wrapped write=wrapped ring=%p\n", ring);
			/* Copy the first part */
			dest = ring->data + ring->end;
			memcpy( dest, data, first_part );

			/* Copy the second part */
			dest = ring->data;
			memcpy( dest, data + first_part, second_part );

			/* Update the pointers */
			ring->end = second_part;
			ring->used += len;
		}
	} else {

		/* 3: The buffer is wrapped and the data can be stored contiguously */
		unsigned char *dest = ring->data + ring->end;

		logging_printf( LOGGING_DEBUG, "ring_buffer_write: buffer=wrapped write=contiguous ring=%p\n", ring );
		memcpy( dest, data, len );

		/* Update the pointers */
		ring->end += len;
		ring->used += len;
	}

ring_buffer_write_end:
	logging_printf( LOGGING_DEBUG, "ring_buffer_write: stage=end len=%zu, ring_buffer=%p,data=%p,size=%zu, start=%zu, end=%zu, used=%zu\n", len, ring, ring->data,ring->size, ring->start, ring->end, ring->used);
	ring_buffer_unlock( ring );

	return return_val;
}

unsigned char *ring_buffer_read( ring_buffer_t *ring, size_t len , int advance)
{
	unsigned char *dest = NULL;
	unsigned char *src = NULL;
	size_t first_part = 0;
	size_t second_part = 0;


	if( ! ring ) goto ring_buffer_read_end;
	if( len == 0 ) goto ring_buffer_read_end;

	ring_buffer_lock( ring );

	logging_printf( LOGGING_DEBUG, "ring_buffer_read: ring=%p,len=%zu,advance=%d\n", ring, len, advance);

	if( ! ring->data ) goto ring_buffer_read_end;
	if( ring->used == 0 ) goto ring_buffer_read_end;
	if( len > ring->used ) goto ring_buffer_read_end;

	dest = ( unsigned char * ) X_MALLOC( len );
	if( ! dest )
	{
		logging_printf( LOGGING_ERROR, "ring_buffer_read: Insufficient memory to allocate read output buffer\n");
		goto ring_buffer_read_end;
	}

	memset( dest, 0, len );

	/* 1: Data is stored contiguously */
	if( ring->start < ring->end )
	{
		src = ring->data + ring->start;

		memcpy( dest, src, len );

		if( advance == RING_YES )
		{
			/* Update the pointers */
			ring->start += len;
			ring->used -= len;
			if( ring->start >= ring->size )
			{
				ring->start = 0;
			}
		}
	} else {

	/* 2: Data is split */

		/* Copy the first part */
		src = ring->data + ring->start;
		first_part = ring->size - ring->start;
		memcpy( dest,  src, first_part );
		
		/* Copy the second part */
		src = ring->data;
		second_part = len - first_part;
		memcpy( dest + first_part, src, second_part );

		if( advance == RING_YES )
		{
			/* Update the pointers */
			ring->used -= len;
			ring->start = second_part;
		}
	}

ring_buffer_read_end:
	ring_buffer_unlock( ring );

	return dest;
}

char ring_buffer_read_byte( ring_buffer_t *ring, uint8_t *return_byte, int advance )
{
	char return_status = -1;

	if(! return_byte ) goto ring_buffer_read_byte_end;

	*return_byte = 0;
	if(! ring ) goto ring_buffer_read_byte_end;

	ring_buffer_lock( ring );

	logging_printf( LOGGING_DEBUG, "ring_buffer_read_byte: ring=%p\tused=%zu\tstart=%zu\tend=%zu\n", ring, ring->used, ring->start, ring->end);

	// No data available
	if( ring->used == 0 ) goto ring_buffer_read_byte_end;
	
	*return_byte = ring->data[ring->start];

	if( advance == RING_YES )
	{
		ring->start += 1;
		ring->used -= 1;
		if( ring->start >= ring->size ) ring->start = 0;
	}

	return_status = 0;

ring_buffer_read_byte_end:
	ring_buffer_unlock( ring );
	return return_status;
}

int ring_buffer_resize( ring_buffer_t *ring, size_t new_size )
{
	unsigned char *new_data = NULL;
	unsigned char *old_data = NULL;
	size_t bytes_used = 0;

	if( ! ring ) return 0;

	new_data = ( unsigned char * )X_MALLOC( new_size );
	if( ! new_data )
	{
		logging_printf( LOGGING_DEBUG, "ring_buffer_resize: Insufficient memory to resize\n");
		return 0;
	}

	memset( new_data, 0, new_size );

	bytes_used = ring_buffer_used( ring );

	old_data = ring_buffer_read( ring, bytes_used, RING_NO);
	if( old_data )
	{
		memcpy( new_data, old_data, bytes_used );
		ring_buffer_lock( ring );
		X_FREE( ring->data );
		ring->data = new_data;
		ring->start = 0;
		ring->end = bytes_used;
		ring->size = new_size;
		ring->used = bytes_used;
		ring_buffer_unlock( ring );

		X_FREE( old_data );
	} else {
		ring_buffer_reset( ring, new_size );
	}
		
	return 1;
}

unsigned char *ring_buffer_drain( ring_buffer_t *ring , size_t *len)
{
	unsigned char *data = NULL;
	*len = 0;
	if( ! ring ) return NULL;

	*len = ring_buffer_used( ring );
	data = ring_buffer_read( ring, *len, RING_YES );

	return data;
}

int ring_buffer_available( ring_buffer_t *ring, size_t requested )
{
	if( ! ring ) return 0;
	if( requested == 0 ) return 0;

	return( requested <= ring->used );
}

int ring_buffer_compare( ring_buffer_t *ring, const char *compare, size_t compare_len )
{
	int ret = 0;
	unsigned char *data = NULL;

	if( ! ring ) return -1;

	data = ring_buffer_read( ring, ring->used, RING_NO );

	if( ! data ) return -1;

	if( compare_len > 0 )
	{
		ret = strncmp( (const char *)data, compare , compare_len);
	} else {
		ret = strcmp( (const char *)data, compare );
	}

	X_FREE(data);
	return ret;
}

int ring_buffer_char_compare( ring_buffer_t *ring, char compare, size_t index )
{
	int ret = 0;
	unsigned char *data = NULL;

	if( ! ring ) return 0;

	if( index > ring->used ) return 0;

	data = ring_buffer_read( ring, ring->used, RING_NO );

	if(! data ) return 0;

	ret = ( data[index] == compare );

	X_FREE(data);

	return ret;
}

void ring_buffer_advance( ring_buffer_t *ring, size_t steps )
{
	unsigned char *data = NULL;
	size_t real_steps = 0;

	if( ! ring ) return;
	if( steps == 0 ) return;

	real_steps = MIN( steps, ring->used );

	data = ring_buffer_read( ring, real_steps, RING_YES );
	X_FREE(data);
}

size_t ring_buffer_get_size( ring_buffer_t *ring )
{
	size_t ring_buffer_size = 0;

	if( ! ring ) return 0;

	ring_buffer_lock( ring );
	ring_buffer_size = ring->size;
	ring_buffer_unlock( ring );
	return ring_buffer_size;
}

