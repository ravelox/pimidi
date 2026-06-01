/*
   This file is part of raveloxmidi.

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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <pthread.h>

#include "config.h"

#include "utils.h"
#include "raveloxmidi_config.h"

#define INSIDE_LOGGING
#include "logging.h"

extern int errno;
static name_map_t loglevel_map[] = {
	{ "DEBUG" , 0 },
	{ "INFO", 1 },
	{ "NORMAL", 2 },
	{ "WARNING", 3 },
	{ "ERROR", 4 },
	{ NULL, -1}
};

static pthread_mutex_t	logging_mutex = PTHREAD_MUTEX_INITIALIZER;

static volatile sig_atomic_t logging_reopen_requested = 0;

static void logging_sighup_handler(int sig)
{
	logging_reopen_requested = 1;
}

static char *logging_file_name = NULL;
static FILE *logging_fp = NULL;
static unsigned char prefix_disabled = 0;

void logging_lock( void )
{
	X_MUTEX_LOCK( &logging_mutex );
}

void logging_unlock( void )
{
	X_MUTEX_UNLOCK( &logging_mutex );
}

int logging_name_to_value(name_map_t *map, const char *name)
{
	int value = -1;
	int i = 0;

	if( ! map ) return value;
	if( ! name ) return value;

	while( map[i].name != NULL )
	{
		if( strcasecmp(name, map[i].name) == 0)
		{
			value = map[i].value;
			break;
		}
		i++;
	}

	return value;
}

char *logging_value_to_name(name_map_t *map, int value)
{
	char *name = NULL;
	int i = 0;

	if( ! map ) return NULL;
	if( value < 0 ) return NULL;

	while( map[i].name != NULL )
	{
		if( map[i].value == value )
		{
			name = map[i].name;
			break;
		}
		i++;
	}

	return name;
}

void logging_prefix_disable( void )
{
	logging_lock();
	prefix_disabled = 1;
	logging_unlock();
}

void logging_prefix_enable( void )
{
	logging_lock();
	prefix_disabled = 0;
	logging_unlock();
}

void logging_printf(int level, const char *format, ...)
{
	FILE *current_logging_fp = NULL;
	va_list ap;

	if( logging_enabled == 0 ) return;

	logging_lock();

	if( level < logging_threshold )
	{
		logging_unlock();
		return;
	}

	if( logging_reopen_requested && logging_file_name )
	{
		logging_reopen_requested = 0;
		if( logging_fp != stderr ) fclose( logging_fp );
		logging_fp = fopen( logging_file_name, "a+" );
		if( !logging_fp ) logging_fp = stderr;
	}

	current_logging_fp = ( logging_fp ? logging_fp : stderr );

	if( ! prefix_disabled )
	{
		struct timeval tv;
		struct timezone tz;

		gettimeofday( &tv, &tz);

		fprintf( current_logging_fp , "[%lu.%lu]\t[tid=%lu]\t%s: ", tv.tv_sec, tv.tv_usec, pthread_self(), logging_value_to_name( loglevel_map, level ) );
	}

	va_list ap_copy;
	va_start(ap, format);
	va_copy(ap_copy, ap);

	if( vfprintf( current_logging_fp, format, ap ) < 0
	 || fflush( current_logging_fp ) != 0 )
	{
	    vfprintf( stderr, format, ap_copy );
	}

	va_end(ap_copy);
	va_end(ap);

	logging_unlock();
}

void logging_init(void)
{
	char *name = NULL;

	pthread_mutex_init( &logging_mutex, NULL );

	logging_lock();

	if( is_yes( config_string_get("logging.enabled") ) )
	{
		logging_threshold = logging_name_to_value( loglevel_map, config_string_get("logging.log_level") ) ;

		// Disable logging to a file if readonly option is set
		if( is_no( config_string_get("readonly") ) )
		{
			name = config_string_get("logging.log_file") ;
			if( name )
			{
				logging_file_name = X_STRDUP( name );
				if( logging_file_name )
				{
					// Check to see if the logging_fp already has a value.
					// If it does, fclose() for safety.
					if(logging_fp && logging_fp != stderr) fclose( logging_fp );
					logging_fp = fopen( logging_file_name , "a+" );
				}
				if( ! logging_fp ) logging_fp = stderr;
			}
		}

		if( ! logging_fp )
		{
			logging_file_name = NULL;
			logging_fp = stderr;
		}
		
		
		logging_enabled = 1;
	}

	signal( SIGHUP, logging_sighup_handler );

	if( is_yes( config_string_get("logging.hex_dump") ) )
	{
		logging_hex_dump = 1;
	}

	logging_unlock();
}

void logging_teardown(void)
{
	logging_lock();

	if( logging_fp && logging_fp != stderr )
	{
		fclose( logging_fp );
	}
	logging_fp = NULL;

	if( logging_file_name )
	{
		X_FREE( logging_file_name );
		logging_file_name = NULL;
	}

	logging_enabled = 0;
	
	logging_unlock();

	pthread_mutex_destroy( &logging_mutex );
}

int logging_get_threshold( void )
{
	int return_value = 0;

	logging_lock();
	return_value = logging_threshold;
	logging_unlock();

	return return_value;
}
