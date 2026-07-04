/*
   raveloxmidi - rtpMIDI implementation for sending NoteOn/Off MIDI events

   Copyright (C) 2014 Dave Kelly

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

#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "raveloxmidi.h"

static raveloxmidi_context_t *signal_context = NULL;

static void usage( void )
{
	fprintf( stderr, "Usage:\n" );
	fprintf( stderr, "\traveloxmidi [-c filename] [-d] [-i] [-R] [-N] [-P filename] [-C] [-h]\n" );
	fprintf( stderr, "\traveloxmidi [--config filename] [--debug] [--info] [--readonly] [--nodaemon] [--pidfile filename] [--dumpconfig] [--version] [--help]\n" );
}

static void shutdown_handler( int sig )
{
	(void)sig;

	if( signal_context ) raveloxmidi_context_request_stop( signal_context );
}

static int set_config_or_fail( raveloxmidi_context_t *context, const char *key, const char *value )
{
	raveloxmidi_status_t status = RAVELOXMIDI_OK;

	status = raveloxmidi_context_set_config( context, key, value );
	if( status != RAVELOXMIDI_OK )
	{
		fprintf( stderr, "Unable to set configuration %s\n", key );
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int main( int argc, char *argv[] )
{
	raveloxmidi_context_t *context = NULL;
	raveloxmidi_status_t status = RAVELOXMIDI_OK;
	const char *config_value = NULL;
	int ret = EXIT_SUCCESS;
	int dump_config = 0;
	int debug_requested = 0;
	static struct option long_options[] = {
		{"config", required_argument, NULL, 'c'},
		{"debug", no_argument, NULL, 'd'},
		{"info", no_argument, NULL, 'i'},
		{"nodaemon", no_argument, NULL, 'N'},
		{"pidfile", required_argument, NULL, 'P'},
		{"readonly", no_argument, NULL, 'R'},
		{"dumpconfig", no_argument, NULL, 'C'},
		{"version", no_argument, NULL, 'v'},
		{"help", no_argument, NULL, 'h'},
		{0, 0, 0, 0}
	};
	const char *short_options = "c:dihNP:RCv";

	status = raveloxmidi_context_create( &context );
	if( status != RAVELOXMIDI_OK )
	{
		fprintf( stderr, "Unable to create raveloxmidi context\n" );
		return EXIT_FAILURE;
	}

	while( 1 )
	{
		int c = getopt_long( argc, argv, short_options, long_options, NULL );
		if( c == -1 ) break;

		switch( c )
		{
			case '?':
				usage();
				ret = EXIT_FAILURE;
				goto cleanup;
			case 'c':
				status = raveloxmidi_context_set_config_file( context, optarg );
				if( status != RAVELOXMIDI_OK )
				{
					fprintf( stderr, "Unable to load configuration file %s\n", optarg );
					ret = EXIT_FAILURE;
					goto cleanup;
				}
				break;
			case 'i':
				if( set_config_or_fail( context, "logging.enabled", "yes" ) != EXIT_SUCCESS )
				{
					ret = EXIT_FAILURE;
					goto cleanup;
				}
				if( set_config_or_fail( context, "logging.log_level", "info" ) != EXIT_SUCCESS )
				{
					ret = EXIT_FAILURE;
					goto cleanup;
				}
				break;
			case 'd':
				if( set_config_or_fail( context, "logging.enabled", "yes" ) != EXIT_SUCCESS )
				{
					ret = EXIT_FAILURE;
					goto cleanup;
				}
				if( set_config_or_fail( context, "logging.log_level", "debug" ) != EXIT_SUCCESS )
				{
					ret = EXIT_FAILURE;
					goto cleanup;
				}
				debug_requested = 1;
				break;
			case 'h':
				usage();
				ret = EXIT_SUCCESS;
				goto cleanup;
			case 'v':
				fprintf( stderr, "raveloxmidi (%s)\n", raveloxmidi_version() );
				ret = EXIT_SUCCESS;
				goto cleanup;
			case 'N':
				if( set_config_or_fail( context, "run_as_daemon", "no" ) != EXIT_SUCCESS )
				{
					ret = EXIT_FAILURE;
					goto cleanup;
				}
				break;
			case 'P':
				if( set_config_or_fail( context, "daemon.pid_file", optarg ) != EXIT_SUCCESS )
				{
					ret = EXIT_FAILURE;
					goto cleanup;
				}
				break;
			case 'R':
				if( set_config_or_fail( context, "readonly", "yes" ) != EXIT_SUCCESS )
				{
					ret = EXIT_FAILURE;
					goto cleanup;
				}
				break;
			case 'C':
				dump_config = 1;
				if( set_config_or_fail( context, "logging.enabled", "yes" ) != EXIT_SUCCESS )
				{
					ret = EXIT_FAILURE;
					goto cleanup;
				}
				if( set_config_or_fail( context, "logging.log_level", "debug" ) != EXIT_SUCCESS )
				{
					ret = EXIT_FAILURE;
					goto cleanup;
				}
				break;
		}
	}

	if( dump_config || debug_requested )
	{
		raveloxmidi_context_dump_config( context );
		if( dump_config )
		{
			ret = EXIT_SUCCESS;
			goto cleanup;
		}
	}

	status = raveloxmidi_context_get_config( context, "logging.log_level", &config_value );
	if( ( status == RAVELOXMIDI_OK ) && config_value && ( strcasecmp( config_value, "debug" ) == 0 ) && ! debug_requested )
	{
		raveloxmidi_context_dump_config( context );
	}

	status = raveloxmidi_context_get_config( context, "network.bind_address", &config_value );
	if( ( status != RAVELOXMIDI_OK ) || ! config_value || ( *config_value == '\0' ) )
	{
		fprintf( stderr, "No network.bind_address configuration is set\n" );
		ret = EXIT_FAILURE;
		goto cleanup;
	}

	signal_context = context;
	signal( SIGINT, shutdown_handler );
	signal( SIGTERM, shutdown_handler );
	signal( SIGUSR2, shutdown_handler );

	status = raveloxmidi_context_start( context );
	if( status != RAVELOXMIDI_OK )
	{
		fprintf( stderr, "Unable to start raveloxmidi\n" );
		ret = EXIT_FAILURE;
		goto cleanup;
	}

	status = raveloxmidi_context_wait( context );
	if( status != RAVELOXMIDI_OK )
	{
		fprintf( stderr, "Unable to wait for raveloxmidi shutdown\n" );
		ret = EXIT_FAILURE;
		goto cleanup;
	}

	ret = EXIT_SUCCESS;

cleanup:
	signal_context = NULL;
	raveloxmidi_context_free( &context );
	return ret;
}
