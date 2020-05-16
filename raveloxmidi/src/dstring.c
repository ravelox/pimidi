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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "utils.h"
#include "logging.h"

#include "dstring.h"

static void dstring_lock( dstring_t *dstring )
{
	if( ! dstring ) return;
	pthread_mutex_lock( &(dstring->lock) );
}

static void dstring_unlock( dstring_t *dstring )
{
	if( ! dstring ) return;
	pthread_mutex_unlock( &(dstring->lock) );
}

dstring_t *dstring_create( size_t block_size )
{
	dstring_t *new_string = NULL;

	if( block_size == 0 ) return NULL;

	new_string = ( dstring_t *)malloc( sizeof( dstring_t ) );

	if( ! new_string )
	{
		logging_printf( LOGGING_DEBUG, "dstring_create: Insufficient memory to create new string\n");
		return NULL;
	}

	memset( new_string, 0, sizeof( dstring_t ) );
	new_string->num_blocks = 1;
	new_string->block_size = block_size;
	new_string->data = (unsigned char *)malloc( new_string->num_blocks * new_string->block_size );
	memset( new_string->data, 0, new_string->num_blocks * new_string->block_size );
	pthread_mutex_init( &(new_string->lock), NULL );

	return new_string;
}

int dstring_reset( dstring_t *dstring )
{
	if( ! dstring ) return 0;

	dstring_lock( dstring );
	if( dstring->data )
	{
		free( dstring->data );
	}
	dstring->num_blocks = 1;
	dstring->data = (unsigned char *)malloc( dstring->num_blocks * dstring->block_size );
	dstring_unlock( dstring );
	return 1;
}

void dstring_destroy( dstring_t **dstring )
{
	if( ! dstring ) return;

	dstring_reset( *dstring );
	pthread_mutex_destroy( &((*dstring)->lock) );
	free( *dstring);
	*dstring = NULL;
}

void dstring_dump( dstring_t *dstring )
{
	DEBUG_ONLY;

	if( ! dstring ) return;
	logging_printf(LOGGING_DEBUG, "dstring: buffer=%p, len=%u, blocks=%u\n", dstring, strlen( dstring->data ), dstring->num_blocks );
}

size_t dstring_append( dstring_t *dstring, char *in_string )
{
	size_t ret = 0;
	size_t current_alloc = 0;

	if( ! dstring ) return ret;
	if( ! in_string ) return ret;

	dstring_lock( dstring );
	
	current_alloc = dstring->num_blocks * dstring->block_size;

	logging_printf( LOGGING_DEBUG, "dstring_append: current_alloc=%zu, data len=%zu, in len=%zu\n", current_alloc, strlen(dstring->data), strlen(in_string) );

	if( strlen( dstring->data ) + strlen( in_string ) >= current_alloc )
	{
		char *new_dstring_data = NULL;
		size_t new_block_count = 0;
		size_t new_alloc = 0;

		new_alloc = strlen( dstring->data ) + strlen( in_string ) + 1;

		new_block_count = ( new_alloc / dstring->block_size ) + 1;
		new_dstring_data = (char *)realloc( dstring->data, new_block_count * dstring->block_size );

		if( ! new_dstring_data )
		{
			logging_printf( LOGGING_DEBUG, "dstring_write: Insufficient memory to expand dstring data\n");
			goto dstring_write_end;
		}

		// Initialise the new memory
		memset( new_dstring_data + strlen(dstring->data), 0, strlen( in_string ) + 1 );

		dstring->num_blocks = new_block_count;
		dstring->data = new_dstring_data;
	}


	strcat( dstring->data, in_string );
	dstring_dump( dstring );

dstring_write_end:
	dstring_unlock( dstring );

	return ret;
}

unsigned char *dstring_value( dstring_t *dstring )
{
	unsigned char *out_buffer = NULL;

	if( ! dstring ) return NULL;

	dstring_lock( dstring );

	out_buffer = dstring->data;
	logging_printf( LOGGING_DEBUG, "dstring_value: out_buffer=%p\n", out_buffer );

	dstring_unlock(dstring);

	return out_buffer;
}
