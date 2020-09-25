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

#include <pthread.h>

#include "data_context.h"

#include "utils.h"
#include "logging.h"

void data_context_lock( data_context_t *context )
{
	if( ! context ) return;
	X_MUTEX_LOCK( &(context->lock) );
}

void data_context_unlock( data_context_t *context )
{
	if( ! context ) return;
	X_MUTEX_UNLOCK( &(context->lock ) );
}

data_context_t *data_context_create( data_context_destroy_func_t destroy_func )
{
	data_context_t *new_context = NULL;

	new_context = (data_context_t *)X_MALLOC( sizeof( data_context_t ) );

	if( ! new_context )
	{
		logging_printf( LOGGING_ERROR, "data_context_create: Unable to create context\n");
		return NULL;
	}

	new_context->data = NULL;
	new_context->reference = 0;
	pthread_mutex_init( &(new_context->lock), NULL );
	new_context->destroy_func = destroy_func;

	logging_printf( LOGGING_DEBUG, "data_context_create: context=%p\n", new_context );
	return new_context;
}

void data_context_destroy( data_context_t **context )
{
	if( ! context ) return;
	if( ! *context ) return;

	data_context_lock( *context );

	logging_printf( LOGGING_DEBUG, "data_context_destroy: context=%p\n", *context);
	if( (*context)->destroy_func )
	{
		(*context)->destroy_func( (*context)->data );
		(*context)->data = NULL;
	}

	(*context)->reference = 0;
	data_context_unlock( *context );

	pthread_mutex_destroy( &((*context)->lock) );

	X_FREE( *context );
	*context = NULL;
}

void data_context_acquire( data_context_t *context )
{
	if(! context ) return;
	
	data_context_lock( context );
	context->reference += 1;
	logging_printf( LOGGING_DEBUG, "data_context_acquire: context=%p: ref=%zu\n", context, context->reference );
	data_context_unlock( context );
}

void data_context_release( data_context_t **context )
{
	uint32_t ref_count = 0;

	if(! context) return;
	if(! *context ) return;

	data_context_lock( *context );
	(*context)->reference -= 1;
	ref_count = (*context)->reference;
	logging_printf( LOGGING_DEBUG, "data_context_release: context=%p: ref=%zu\n", *context, (*context)->reference );
	data_context_unlock( *context );

	if( ref_count == 0 )
	{
		data_context_destroy( context );
	}
}
