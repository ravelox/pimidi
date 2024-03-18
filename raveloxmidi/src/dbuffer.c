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

#include "dbuffer.h"

static void dbuffer_lock( dbuffer_t *dbuffer )
{
	if( ! dbuffer ) return;
	X_MUTEX_LOCK( &(dbuffer->lock) );
}

static void dbuffer_unlock( dbuffer_t *dbuffer )
{
	if( ! dbuffer ) return;
	X_MUTEX_UNLOCK( &(dbuffer->lock) );
}

dbuffer_t *dbuffer_create( size_t block_size )
{
	dbuffer_t *new_buffer = NULL;

	if( block_size == 0 ) return NULL;

	new_buffer = ( dbuffer_t *)X_MALLOC( sizeof( dbuffer_t ) );

	if( ! new_buffer )
	{
		logging_printf( LOGGING_DEBUG, "dbuffer_create: Insufficient memory to create new buffer\n");
		return NULL;
	}

	memset( new_buffer, 0, sizeof( dbuffer_t ) );
	new_buffer->num_blocks = 0;
	new_buffer->len = 0;
	new_buffer->block_size = block_size;
	pthread_mutex_init( &(new_buffer->lock), NULL );

	return new_buffer;
}

int dbuffer_reset( dbuffer_t *dbuffer )
{
	if( ! dbuffer ) return 0;

	dbuffer_lock( dbuffer );
	if( dbuffer->data )
	{
		X_FREE( dbuffer->data );
	}
	dbuffer->data = NULL;
	dbuffer->len = 0;
	dbuffer->num_blocks = 0;
	dbuffer_unlock( dbuffer );
	return 1;
}

void dbuffer_destroy( dbuffer_t **dbuffer )
{
	if( ! dbuffer ) return;

	dbuffer_reset( *dbuffer );
	pthread_mutex_destroy( &((*dbuffer)->lock) );
	X_FREE( *dbuffer);
	*dbuffer = NULL;
}

size_t dbuffer_len( dbuffer_t *dbuffer )
{
	size_t ret = 0;

	if( ! dbuffer ) return ret;
	if( ! dbuffer->data ) return ret;

	dbuffer_lock( dbuffer );
	ret = dbuffer->len;
	dbuffer_unlock( dbuffer);
	return ret;
}

void dbuffer_dump( dbuffer_t *dbuffer )
{
	DEBUG_ONLY;

	if( ! dbuffer ) return;
	logging_printf(LOGGING_DEBUG, "dbuffer: buffer=%p, len=%u, blocks=%u\n", dbuffer, dbuffer->len, dbuffer->num_blocks );
}

size_t dbuffer_write( dbuffer_t *dbuffer, unsigned char *in_buffer, size_t in_buffer_len )
{
	size_t ret = 0;

	if( ! dbuffer ) return ret;
	if( ! in_buffer ) return ret;

	dbuffer_lock( dbuffer );
	
	if( dbuffer->len < ( dbuffer->len + in_buffer_len + 1) )
	{
		unsigned char *new_dbuffer_data = NULL;
		size_t new_block_count = 0;

		new_block_count = ( ( dbuffer->len + in_buffer_len + 1 ) / dbuffer->block_size ) + 1 ;
		new_dbuffer_data = (unsigned char *)X_REALLOC( dbuffer->data, new_block_count * dbuffer->block_size );

		if( ! new_dbuffer_data )
		{
			logging_printf( LOGGING_DEBUG, "dbuffer_write: Insufficient memory to expand dbuffer data\n");
			goto dbuffer_write_end;
		}

		dbuffer->num_blocks = new_block_count;
		dbuffer->data = new_dbuffer_data;

		// Initialise the new memory
		memset( dbuffer->data + dbuffer->len, 0, in_buffer_len + 1 );

	}

	memcpy( dbuffer->data + dbuffer->len, in_buffer, in_buffer_len );
	dbuffer->len += in_buffer_len;
	dbuffer_dump( dbuffer );
dbuffer_write_end:
	dbuffer_unlock( dbuffer );

	return ret;
}

size_t dbuffer_read( dbuffer_t *dbuffer, unsigned char **out_buffer )
{
	size_t ret = 0;

	if( ! dbuffer ) return ret;
	if( ! out_buffer ) return ret;

	dbuffer_lock( dbuffer );

	dbuffer_dump( dbuffer );

	*out_buffer = ( unsigned char * ) X_MALLOC( dbuffer->len );
	memset( *out_buffer, 0, dbuffer->len );
	if( ! *out_buffer )
	{
		logging_printf( LOGGING_DEBUG, "dbuffer_read: Insufficient memory to create output buffer\n");
		goto dbuffer_read_end;
	}

	memcpy( *out_buffer, dbuffer->data, dbuffer->len );
	ret = dbuffer->len;

dbuffer_read_end:
	dbuffer_unlock(dbuffer);

	return ret;
}
