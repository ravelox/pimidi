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

#include "data_queue.h"
#include "data_context.h"

#include "utils.h"
#include "logging.h"

void data_queue_lock( data_queue_t *queue )
{
	if( ! queue ) return;
	X_MUTEX_LOCK( &(queue->lock) );
}

void data_queue_unlock( data_queue_t *queue )
{
	if( ! queue ) return;
	X_MUTEX_UNLOCK( &(queue->lock) );
}

static void data_queue_wake_handler( data_queue_t *queue )
{
	if( ! queue ) return;
	pthread_cond_signal( &(queue->data_signal) );
}

data_queue_item_t *data_queue_item_create( void )
{
	data_queue_item_t *new_item = NULL;

	new_item = (data_queue_item_t *)X_MALLOC( sizeof( data_queue_item_t ) );

	if( !new_item )
	{
		logging_printf(LOGGING_ERROR, "data_queue_item_create: Insufficient memory to create new data_queue_item\n");
		return NULL;
	}
	new_item->data = NULL;
	new_item->next = NULL;

	return new_item;
}

data_queue_t *data_queue_create( char *name, data_queue_action_func_t action )
{
	data_queue_t *new_queue = NULL;

	new_queue = ( data_queue_t * ) X_MALLOC( sizeof( data_queue_t ) );
	
	if( ! new_queue )
	{
		logging_printf( LOGGING_ERROR, "data_queue_create: Insufficient memory to create new queue\n");
		return NULL;
	}

	memset( new_queue, 0, sizeof( data_queue_t ) );

	pthread_mutex_init( &(new_queue->lock), NULL );

	data_queue_lock( new_queue );

	pthread_cond_init( &(new_queue->data_signal), NULL );
	new_queue->state = DATA_QUEUE_EMPTY;
	new_queue->top = NULL;
	new_queue->bottom = NULL;
	new_queue->shutdown = DATA_QUEUE_CONTINUE;
	new_queue->name = X_STRDUP( name );
	new_queue->counter = 0;

	if( action )
	{
		new_queue->action = action;
	}

	data_queue_unlock( new_queue );

	return new_queue;
}

unsigned char data_queue_shutdown_status( data_queue_t *queue )
{
	unsigned char shutdown_status = 0;

	if( ! queue ) return 0;

	data_queue_lock( queue );
	shutdown_status = queue->shutdown;
	data_queue_unlock( queue );

	return shutdown_status;
}

void data_queue_destroy( data_queue_t **queue )
{
	data_queue_item_t *item = NULL;

	if( ! queue ) return;
	if( ! *queue ) return;

	data_queue_lock( *queue );

	while( (*queue)->top )
	{
		item = (*queue)->top;
		(*queue)->top = (*queue)->top->next;
		item->next = NULL;
		item->data = NULL;
		X_FREENULL( "data_queue_destroy:item", (void **)&item );
	}

	if( (*queue)->name )
	{
		X_FREENULL( "data_queue_destroy:queue->name", (void **) &( (*queue)->name ) );
	}

	if( (*queue)->action )
	{
		(*queue)->action = NULL;
	}

	data_queue_unlock( *queue );

	pthread_cond_destroy( &((*queue)->data_signal) );
	pthread_mutex_destroy( &((*queue)->lock) );

	X_FREENULL( "data_queue_destroy:queue", (void **)queue );
}

void data_queue_add( data_queue_t *queue, void *data, void *context )
{
	data_queue_item_t *new_item;
	unsigned char old_state;

	if( ! queue ) return;
	if( ! data ) return;

	new_item = data_queue_item_create();
	
	if( ! new_item )
	{
		logging_printf(LOGGING_DEBUG, "data_queue_add: Insufficient memory to create new data_queue_item\n");
		return;
	}

	new_item->data = data;
	new_item->context = context;

	data_queue_lock( queue );

	logging_printf( LOGGING_DEBUG, "data_queue_add: Adding item=%p to [%s]\n", new_item, (queue->name?queue->name:"unknown") );

	if( queue->top == NULL )
	{
		queue->top = new_item;
	}

	if( queue->bottom )
	{
		queue->bottom->next = new_item;
	}

	queue->bottom = new_item;
	queue->bottom->next = NULL;

	old_state = queue->state;
	queue->state = DATA_QUEUE_USED;

// If the queue was originally empty, signal the queue handler to wake up
	if( old_state == DATA_QUEUE_EMPTY )
	{
		data_queue_wake_handler( queue );
	}

	logging_printf( LOGGING_DEBUG, "data_queue_add: queue=%p, name=[%s], top=%p, bottom=%p, id=%lu, state=%u\n",
		queue, (queue->name ? queue->name : "unknown" ), queue->top, queue->bottom, queue->counter, queue->state );


	data_queue_unlock( queue );
}

/* Must be locked by caller */
data_queue_item_t* data_queue_get( data_queue_t *queue )
{
	data_queue_item_t *item = NULL;

	if( ! queue ) return NULL;

	item = queue->top;
	logging_printf( LOGGING_DEBUG, "data_queue_get: item=%p\n", item );

	if( queue->top )
	{
		queue->top = queue->top->next;
	}

	if( ! queue->top )
	{
		queue->bottom = NULL;
	}

	queue->state = ( queue->top ? DATA_QUEUE_USED: DATA_QUEUE_EMPTY );

	return item;
}

static void data_queue_wait_for_data( data_queue_t *queue )
{
	if( ! queue ) return;
	pthread_cond_wait( &(queue->data_signal), &(queue->lock) );
}
	
static void *data_queue_handler( void *data )
{
	data_queue_t *queue = NULL;
	data_queue_item_t *item = NULL;

	if( ! data )
	{
		logging_printf( LOGGING_DEBUG, "data_queue_handler: No queue structure provided\n");
		return NULL;
	}

	queue = ( data_queue_t *)data;

	logging_printf( LOGGING_DEBUG, "data_queue_handler: [%s] thread started\n", (queue->name ? queue->name : "unknown") );

	// Eternal loop until told to shut down
	while( 1 )
	{
		data_queue_lock( queue );
	
		while( ( queue->state == DATA_QUEUE_EMPTY ) && ( queue->shutdown == DATA_QUEUE_CONTINUE ) )
		{
			data_queue_wait_for_data( queue );
		}

		if( queue->shutdown != DATA_QUEUE_CONTINUE )
		{
			data_queue_unlock( queue );
			break;
		}

		item = data_queue_get( queue );

		if( item )
		{
			/* Do something with the item */
			logging_printf( LOGGING_DEBUG, "data_queue_handler: [%s] got item=%p\n", (queue->name ? queue->name : "unknown") , item );

			if( queue->action )
			{
				queue->action( item->data, item->context );
			}

			item->data = NULL;
			item->next = NULL;

			X_FREENULL( "data_queue_handler:item", (void **)&item);
		}

queue_handler_loop_end:
		data_queue_unlock( queue );
	}

	logging_printf( LOGGING_DEBUG, "data_queue_handler: [%s] stopped\n", ( queue->name ? queue->name : "unknown") );

	return NULL;
}

void data_queue_stop( data_queue_t *queue )
{
	if( ! queue ) return;

	logging_printf( LOGGING_DEBUG, "data_queue_stop: name=[%s]\n", ( queue->name  ? queue->name : "unknown") );
	data_queue_lock( queue );
	queue->shutdown = 1;
	data_queue_wake_handler( queue );
	data_queue_unlock( queue );
}

void data_queue_start( data_queue_t *queue )
{
	if( ! queue ) return;

	logging_printf( LOGGING_DEBUG, "data_queue_start: name=[%s]\n", ( queue->name  ? queue->name : "unknown") );
	pthread_create( &(queue->queue_thread), NULL, data_queue_handler, queue );
}

void data_queue_join( data_queue_t *queue )
{
	if( ! queue ) return;

	logging_printf( LOGGING_DEBUG, "data_queue_join: name=[%s]\n", ( queue->name  ? queue->name : "unknown") );
	pthread_join( queue->queue_thread, NULL );
}
