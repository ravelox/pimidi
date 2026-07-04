/*
   This file is part of raveloxmidi.

   Copyright (C) 2026 Dave Kelly

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.
*/

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include <raveloxmidi.h>

static int check_status( const char *operation, raveloxmidi_status_t status )
{
	if( status == RAVELOXMIDI_OK ) return 0;

	fprintf( stderr, "%s failed: %d\n", operation, status );
	return 1;
}

int main( int argc, char **argv )
{
	raveloxmidi_context_t *context = NULL;
	const char *bind_address = "127.0.0.1";
	uint8_t note_on[] = { 60, 100 };
	int rc = 0;

	if( argc > 1 ) bind_address = argv[1];

	rc = check_status( "create context", raveloxmidi_context_create( &context ) );
	if( rc ) return rc;

	if( ! rc ) rc = check_status( "set bind address", raveloxmidi_context_set_config( context, "network.bind_address", bind_address ) );
	if( ! rc ) rc = check_status( "disable inbound file sink", raveloxmidi_context_set_config( context, "inbound_midi", "none" ) );
	if( ! rc ) rc = check_status( "start context", raveloxmidi_context_start( context ) );
	if( ! rc ) rc = check_status( "send middle C note on", raveloxmidi_context_send_raw_midi( context, 0x90, note_on, sizeof( note_on ) ) );

	sleep( 1 );

	if( context )
	{
		raveloxmidi_context_stop( context );
		raveloxmidi_context_free( &context );
	}

	return rc;
}
