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

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#include "midi_command.h"
#include "raveloxmidi.h"
#include "raveloxmidi_events.h"

typedef struct callback_test_state_t {
	pthread_mutex_t lock;
	pthread_cond_t signal;
	int called;
	raveloxmidi_event_type_t type;
	uint8_t status;
	uint8_t channel;
	uint8_t data[2];
	size_t data_len;
	void *user_data;
} callback_test_state_t;

static int expect_status( const char *label, raveloxmidi_status_t actual, raveloxmidi_status_t expected )
{
	if( actual == expected ) return 0;

	fprintf( stderr, "%s: expected status %d, got %d\n", label, expected, actual );
	return 1;
}

static void callback_test_handler( raveloxmidi_context_t *context, const raveloxmidi_midi_event_t *event, void *user_data )
{
	callback_test_state_t *state = NULL;

	(void)context;

	if( ! event ) return;
	if( ! user_data ) return;

	state = ( callback_test_state_t * )user_data;

	pthread_mutex_lock( &(state->lock) );
	state->called = 1;
	state->type = event->type;
	state->status = event->status;
	state->channel = event->channel;
	state->data_len = event->data_len;
	state->user_data = user_data;
	if( event->data && event->data_len >= 2 )
	{
		state->data[0] = event->data[0];
		state->data[1] = event->data[1];
	}
	pthread_cond_signal( &(state->signal) );
	pthread_mutex_unlock( &(state->lock) );
}

static int wait_for_callback( callback_test_state_t *state )
{
	struct timespec timeout;
	int wait_status = 0;

	clock_gettime( CLOCK_REALTIME, &timeout );
	timeout.tv_sec += 5;

	pthread_mutex_lock( &(state->lock) );
	while( ! state->called && wait_status == 0 )
	{
		wait_status = pthread_cond_timedwait( &(state->signal), &(state->lock), &timeout );
	}
	pthread_mutex_unlock( &(state->lock) );

	return state->called ? 0 : 1;
}

static int run_callback_delivery_test( raveloxmidi_context_t *context )
{
	callback_test_state_t state;
	midi_command_t *command = NULL;
	const uint8_t note_data[] = { 60, 100 };
	int failed = 0;

	memset( &state, 0, sizeof( state ) );
	pthread_mutex_init( &(state.lock), NULL );
	pthread_cond_init( &(state.signal), NULL );

	if( expect_status( "set callback", raveloxmidi_context_set_midi_event_callback( context, callback_test_handler, &state ), RAVELOXMIDI_OK ) )
	{
		failed = 1;
		goto callback_delivery_done;
	}

	command = midi_command_create();
	if( ! command )
	{
		fprintf( stderr, "unable to create midi command\n" );
		failed = 1;
		goto callback_delivery_done;
	}

	midi_command_set( command, 0, 0x90, note_data, sizeof( note_data ) );
	raveloxmidi_events_publish_midi_command( command, 0x12345678, -1 );

	if( wait_for_callback( &state ) )
	{
		fprintf( stderr, "timed out waiting for callback\n" );
		failed = 1;
		goto callback_delivery_done;
	}

	if( state.type != RAVELOXMIDI_EVENT_NOTE_ON ||
		state.status != 0x90 ||
		state.channel != 0 ||
		state.data_len != 2 ||
		state.data[0] != 60 ||
		state.data[1] != 100 ||
		state.user_data != &state )
	{
		fprintf( stderr, "callback received unexpected event data\n" );
		failed = 1;
	}

callback_delivery_done:
	if( command ) midi_command_destroy( (void **)&command );
	raveloxmidi_context_clear_midi_event_callback( context );
	pthread_cond_destroy( &(state.signal) );
	pthread_mutex_destroy( &(state.lock) );

	return failed;
}

int main( void )
{
	int i;

	if( ! raveloxmidi_version() )
	{
		fprintf( stderr, "raveloxmidi_version returned NULL\n" );
		return 1;
	}

	for( i = 0; i < 100; i++ )
	{
		raveloxmidi_context_t *context = NULL;
		const char *value = NULL;

		if( expect_status( "create", raveloxmidi_context_create( &context ), RAVELOXMIDI_OK ) ) return 1;
		if( ! context )
		{
			fprintf( stderr, "create returned OK without a context\n" );
			return 1;
		}

		if( expect_status( "set network.bind_address", raveloxmidi_context_set_config( context, "network.bind_address", "127.0.0.1" ), RAVELOXMIDI_OK ) ) return 1;
		if( expect_status( "get network.bind_address", raveloxmidi_context_get_config( context, "network.bind_address", &value ), RAVELOXMIDI_OK ) ) return 1;
		if( ! value || strcmp( value, "127.0.0.1" ) != 0 )
		{
			fprintf( stderr, "unexpected network.bind_address value\n" );
			return 1;
		}

		if( expect_status( "set service.name", raveloxmidi_context_set_config( context, "service.name", "raveloxmidi-memcheck" ), RAVELOXMIDI_OK ) ) return 1;
		if( expect_status( "get service.name", raveloxmidi_context_get_config( context, "service.name", &value ), RAVELOXMIDI_OK ) ) return 1;
		if( ! value || strcmp( value, "raveloxmidi-memcheck" ) != 0 )
		{
			fprintf( stderr, "unexpected service.name value\n" );
			return 1;
		}

		if( expect_status( "missing config item", raveloxmidi_context_get_config( context, "memcheck.missing", &value ), RAVELOXMIDI_ERROR_NOT_FOUND ) ) return 1;
		if( value )
		{
			fprintf( stderr, "missing config item returned a value\n" );
			return 1;
		}

		if( i == 0 && run_callback_delivery_test( context ) ) return 1;

		raveloxmidi_context_free( &context );
		if( context )
		{
			fprintf( stderr, "context pointer was not cleared\n" );
			return 1;
		}
	}

	return 0;
}
