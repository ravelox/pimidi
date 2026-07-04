/*
   This file is part of raveloxmidi.

   Copyright (C) 2026 Dave Kelly

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.
*/

#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include <raveloxmidi.h>

static volatile sig_atomic_t stop_requested = 0;

static void handle_signal( int signo )
{
	(void)signo;
	stop_requested = 1;
}

static int check_status( const char *operation, raveloxmidi_status_t status )
{
	if( status == RAVELOXMIDI_OK ) return 0;

	fprintf( stderr, "%s failed: %d\n", operation, status );
	return 1;
}

static void midi_event_callback( raveloxmidi_context_t *context, const raveloxmidi_midi_event_t *event, void *user_data )
{
	size_t i;

	(void)context;
	(void)user_data;

	printf( "event type=%d status=0x%02x channel=%u bytes=", event->type, event->status, event->channel );
	for( i = 0; i < event->data_len; i++ )
	{
		printf( "%s0x%02x", i == 0 ? "" : " ", event->data[i] );
	}
	printf( "\n" );
	fflush( stdout );
}

int main( int argc, char **argv )
{
	raveloxmidi_context_t *context = NULL;
	const char *bind_address = "0.0.0.0";
	int rc = 0;

	if( argc > 1 ) bind_address = argv[1];

	signal( SIGINT, handle_signal );
	signal( SIGTERM, handle_signal );

	rc = check_status( "create context", raveloxmidi_context_create( &context ) );
	if( rc ) return rc;

	if( ! rc ) rc = check_status( "set bind address", raveloxmidi_context_set_config( context, "network.bind_address", bind_address ) );
	if( ! rc ) rc = check_status( "disable inbound file sink", raveloxmidi_context_set_config( context, "inbound_midi", "none" ) );
	if( ! rc ) rc = check_status( "set callback", raveloxmidi_context_set_midi_event_callback( context, midi_event_callback, NULL ) );
	if( ! rc ) rc = check_status( "start context", raveloxmidi_context_start( context ) );

	while( ! rc && ! stop_requested )
	{
		sleep( 1 );
	}

	if( context )
	{
		raveloxmidi_context_stop( context );
		raveloxmidi_context_free( &context );
	}

	return rc;
}
