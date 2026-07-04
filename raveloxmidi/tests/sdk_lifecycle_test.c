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

#include "raveloxmidi.h"

static int expect_status( const char *label, raveloxmidi_status_t actual, raveloxmidi_status_t expected )
{
	if( actual == expected ) return 0;

	fprintf( stderr, "%s: expected status %d, got %d\n", label, expected, actual );
	return 1;
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

		raveloxmidi_context_free( &context );
		if( context )
		{
			fprintf( stderr, "context pointer was not cleared\n" );
			return 1;
		}
	}

	return 0;
}
