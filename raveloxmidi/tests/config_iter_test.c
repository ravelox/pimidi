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

/*
	Multi-value config options (e.g. alsa.output_device.N) are discovered
	by scanning the config table for prefix.<digits> keys rather than
	probing prefix.0, prefix.1, ... and stopping at the first gap. This
	confirms gaps in the suffix sequence are tolerated.
*/

#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "raveloxmidi_config.h"

int main( void )
{
	char *argv[] = { (char *)"config_iter_test", NULL };
	raveloxmidi_config_iter_t *iter = NULL;
	const char *expected[] = { "dev0", "dev1", "dev3", "dev4", "dev5" };
	size_t expected_count = sizeof( expected ) / sizeof( expected[0] );
	size_t seen = 0;
	int failed = 0;

	config_init( 1, argv );

	/* Non-contiguous suffixes: 0, 1, 3, 4, 5 - index 2 is missing. */
	config_add_item( "alsa.output_device.0", "dev0" );
	config_add_item( "alsa.output_device.1", "dev1" );
	config_add_item( "alsa.output_device.3", "dev3" );
	config_add_item( "alsa.output_device.4", "dev4" );
	config_add_item( "alsa.output_device.5", "dev5" );

	/* Must not be picked up: different key that merely shares the prefix text. */
	config_add_item( "alsa.output_device_extra.0", "decoy" );

	/* Must not be picked up: an explicitly blank value is treated as unset. */
	config_add_item( "alsa.output_device.9", "" );

	iter = config_iter_create( "alsa.output_device" );
	if( ! iter )
	{
		fprintf( stderr, "config_iter_create returned NULL\n" );
		return 1;
	}

	while( config_iter_is_set( iter ) )
	{
		const char *value = config_iter_string_get( iter );

		if( seen >= expected_count )
		{
			fprintf( stderr, "more values returned than expected: got extra \"%s\"\n", value ? value : "(null)" );
			failed = 1;
			break;
		}

		if( ! value || strcmp( value, expected[ seen ] ) != 0 )
		{
			fprintf( stderr, "position %zu: expected \"%s\", got \"%s\"\n",
				seen, expected[ seen ], value ? value : "(null)" );
			failed = 1;
		}

		seen++;
		config_iter_next( iter );
	}

	if( seen != expected_count )
	{
		fprintf( stderr, "expected %zu values, got %zu\n", expected_count, seen );
		failed = 1;
	}

	config_iter_destroy( &iter );

	/* An iterator over a prefix with no matches is empty, not NULL, and safe to use. */
	iter = config_iter_create( "no.such.prefix" );
	if( ! iter )
	{
		fprintf( stderr, "config_iter_create returned NULL for an unmatched prefix\n" );
		failed = 1;
	}
	else
	{
		if( config_iter_is_set( iter ) )
		{
			fprintf( stderr, "unmatched prefix unexpectedly reported a set value\n" );
			failed = 1;
		}
		config_iter_destroy( &iter );
	}

	config_teardown();

	return failed;
}
