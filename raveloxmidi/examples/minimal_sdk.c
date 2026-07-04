/*
   This file is part of raveloxmidi.

   Copyright (C) 2026 Dave Kelly

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.
*/

#include <stdio.h>

#include <raveloxmidi.h>

static int check_status( const char *operation, raveloxmidi_status_t status )
{
	if( status == RAVELOXMIDI_OK ) return 0;

	fprintf( stderr, "%s failed: %d\n", operation, status );
	return 1;
}

int main( void )
{
	raveloxmidi_context_t *context = NULL;
	const char *value = NULL;
	int rc = 0;

	printf( "libraveloxmidi version: %s\n", raveloxmidi_version() );

	rc = check_status( "create context", raveloxmidi_context_create( &context ) );
	if( rc ) return rc;

	rc = check_status( "set bind address", raveloxmidi_context_set_config( context, "network.bind_address", "127.0.0.1" ) );
	if( ! rc ) rc = check_status( "get bind address", raveloxmidi_context_get_config( context, "network.bind_address", &value ) );

	if( ! rc ) printf( "network.bind_address=%s\n", value );

	raveloxmidi_context_free( &context );
	return rc;
}
