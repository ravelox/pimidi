/*
   This file is part of raveloxmidi.

   Copyright (C) 2026 Dave Kelly

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

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "data_queue.h"
#include "logging.h"
#include "midi_command.h"
#include "raveloxmidi_events.h"
#include "utils.h"

typedef struct raveloxmidi_event_dispatcher_t {
	raveloxmidi_context_t *context;
	raveloxmidi_midi_event_callback_t callback;
	void *user_data;
	data_queue_t *queue;
	int initialized;
	int running;
} raveloxmidi_event_dispatcher_t;

typedef struct raveloxmidi_event_queue_item_t {
	raveloxmidi_midi_event_t event;
	uint8_t *data;
} raveloxmidi_event_queue_item_t;

static raveloxmidi_event_dispatcher_t event_dispatcher;
static pthread_mutex_t event_dispatcher_lock = PTHREAD_MUTEX_INITIALIZER;

static void raveloxmidi_events_lock( void )
{
	X_MUTEX_LOCK( &event_dispatcher_lock );
}

static void raveloxmidi_events_unlock( void )
{
	X_MUTEX_UNLOCK( &event_dispatcher_lock );
}

static raveloxmidi_event_type_t raveloxmidi_events_map_type( const midi_command_t *command )
{
	enum midi_message_type_t message_type = MIDI_NULL;

	midi_command_map( command, NULL, &message_type );

	switch( message_type )
	{
		case MIDI_NOTE_OFF:
			return RAVELOXMIDI_EVENT_NOTE_OFF;
		case MIDI_NOTE_ON:
			return RAVELOXMIDI_EVENT_NOTE_ON;
		case MIDI_CONTROL_CHANGE:
			return RAVELOXMIDI_EVENT_CONTROL_CHANGE;
		case MIDI_PROGRAM_CHANGE:
			return RAVELOXMIDI_EVENT_PROGRAM_CHANGE;
		default:
			return RAVELOXMIDI_EVENT_RAW_MIDI;
	}
}

static raveloxmidi_event_queue_item_t *raveloxmidi_event_queue_item_create(
	const midi_command_t *command,
	uint32_t originator_ssrc,
	int originator_alsa_device_hash )
{
	raveloxmidi_event_queue_item_t *item = NULL;

	if( ! command ) return NULL;

	item = ( raveloxmidi_event_queue_item_t * ) X_MALLOC( sizeof( raveloxmidi_event_queue_item_t ) );
	if( ! item ) return NULL;

	memset( item, 0, sizeof( raveloxmidi_event_queue_item_t ) );

	if( command->data && command->data_len > 0 )
	{
		item->data = ( uint8_t * ) X_MALLOC( command->data_len );
		if( ! item->data )
		{
			X_FREE( item );
			return NULL;
		}
		memcpy( item->data, command->data, command->data_len );
	}

	item->event.type = raveloxmidi_events_map_type( command );
	item->event.delta = command->delta;
	item->event.status = command->status;
	item->event.channel = command->status < 0xf0 ? ( command->status & 0x0f ) : 0;
	item->event.data = item->data;
	item->event.data_len = command->data_len;
	item->event.originator_ssrc = originator_ssrc;
	item->event.originator_alsa_device_hash = originator_alsa_device_hash;

	return item;
}

static void raveloxmidi_event_queue_item_destroy( raveloxmidi_event_queue_item_t **item )
{
	if( ! item ) return;
	if( ! *item ) return;

	X_FREENULL( "raveloxmidi_event_queue_item_destroy:data", (void **)&((*item)->data) );
	X_FREENULL( "raveloxmidi_event_queue_item_destroy:item", (void **)item );
}

static void raveloxmidi_events_dispatch( void *data, void *context )
{
	raveloxmidi_event_queue_item_t *item = NULL;
	raveloxmidi_context_t *callback_context = NULL;
	raveloxmidi_midi_event_callback_t callback = NULL;
	void *user_data = NULL;

	(void)context;

	if( ! data ) return;
	item = ( raveloxmidi_event_queue_item_t * )data;

	raveloxmidi_events_lock();
	callback_context = event_dispatcher.context;
	callback = event_dispatcher.callback;
	user_data = event_dispatcher.user_data;
	raveloxmidi_events_unlock();

	if( callback ) callback( callback_context, &(item->event), user_data );

	raveloxmidi_event_queue_item_destroy( &item );
}

raveloxmidi_status_t raveloxmidi_events_init( raveloxmidi_context_t *context )
{
	if( ! context ) return RAVELOXMIDI_ERROR_INVALID_ARGUMENT;

	memset( &event_dispatcher, 0, sizeof( event_dispatcher ) );

	event_dispatcher.queue = data_queue_create( "MIDI event callbacks", raveloxmidi_events_dispatch );
	if( ! event_dispatcher.queue )
	{
		return RAVELOXMIDI_ERROR_NO_MEMORY;
	}

	raveloxmidi_events_lock();
	event_dispatcher.context = context;
	event_dispatcher.initialized = 1;
	event_dispatcher.running = 1;
	raveloxmidi_events_unlock();

	data_queue_start( event_dispatcher.queue );

	return RAVELOXMIDI_OK;
}

void raveloxmidi_events_teardown( raveloxmidi_context_t *context )
{
	data_queue_t *queue = NULL;
	int initialized = 0;

	(void)context;

	raveloxmidi_events_lock();
	initialized = event_dispatcher.initialized;
	queue = event_dispatcher.queue;
	event_dispatcher.callback = NULL;
	event_dispatcher.user_data = NULL;
	event_dispatcher.context = NULL;
	event_dispatcher.queue = NULL;
	event_dispatcher.initialized = 0;
	event_dispatcher.running = 0;
	raveloxmidi_events_unlock();

	if( ! initialized ) return;

	if( queue )
	{
		data_queue_stop( queue );
		data_queue_join( queue );
		data_queue_destroy( &queue );
	}

	memset( &event_dispatcher, 0, sizeof( event_dispatcher ) );
}

raveloxmidi_status_t raveloxmidi_events_set_callback( raveloxmidi_context_t *context, raveloxmidi_midi_event_callback_t callback, void *user_data )
{
	if( ! context ) return RAVELOXMIDI_ERROR_INVALID_ARGUMENT;
	if( ! callback ) return RAVELOXMIDI_ERROR_INVALID_ARGUMENT;

	raveloxmidi_events_lock();
	if( ! event_dispatcher.initialized || event_dispatcher.context != context )
	{
		raveloxmidi_events_unlock();
		return RAVELOXMIDI_ERROR_INVALID_STATE;
	}

	event_dispatcher.callback = callback;
	event_dispatcher.user_data = user_data;
	raveloxmidi_events_unlock();

	return RAVELOXMIDI_OK;
}

raveloxmidi_status_t raveloxmidi_events_clear_callback( raveloxmidi_context_t *context )
{
	if( ! context ) return RAVELOXMIDI_ERROR_INVALID_ARGUMENT;

	raveloxmidi_events_lock();
	if( ! event_dispatcher.initialized || event_dispatcher.context != context )
	{
		raveloxmidi_events_unlock();
		return RAVELOXMIDI_ERROR_INVALID_STATE;
	}

	event_dispatcher.callback = NULL;
	event_dispatcher.user_data = NULL;
	raveloxmidi_events_unlock();

	return RAVELOXMIDI_OK;
}

void raveloxmidi_events_publish_midi_command( const midi_command_t *command, uint32_t originator_ssrc, int originator_alsa_device_hash )
{
	raveloxmidi_event_queue_item_t *item = NULL;
	data_queue_t *queue = NULL;
	raveloxmidi_midi_event_callback_t callback = NULL;

	if( ! command ) return;

	raveloxmidi_events_lock();
	callback = event_dispatcher.callback;
	queue = event_dispatcher.queue;
	raveloxmidi_events_unlock();

	if( ! callback || ! queue ) return;

	item = raveloxmidi_event_queue_item_create( command, originator_ssrc, originator_alsa_device_hash );
	if( ! item )
	{
		logging_printf( LOGGING_ERROR, "raveloxmidi_events_publish_midi_command: Unable to allocate event item\n" );
		return;
	}

	data_queue_add( queue, item, NULL );
}
