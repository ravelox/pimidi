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
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>

#include "config.h"

#include "utils.h"
#include "raveloxmidi_config.h"
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

static pthread_mutex_t	logging_mutex;

static int logging_enabled = 0;
static int logging_threshold = 3;
static char *logging_file_name = NULL;
static unsigned char prefix_disabled = 0;

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
	pthread_mutex_lock( &logging_mutex );
	prefix_disabled = 1;
	pthread_mutex_unlock( &logging_mutex );
}

void logging_prefix_enable( void )
{
	pthread_mutex_lock( &logging_mutex );
	prefix_disabled = 0;
	pthread_mutex_unlock( &logging_mutex );
}

void logging_printf(int level, const char *format, ...)
{
	FILE *logging_fp = NULL;
	va_list ap;

	pthread_mutex_lock( &logging_mutex );

	if( logging_enabled == 0 )
	{
		goto logging_end;
	}


	if( level < logging_threshold )
	{
		goto logging_end;
	}

	if( logging_file_name )
	{
		logging_fp = fopen( logging_file_name , "a+" );
		if( !logging_fp )
		{
			logging_fp = stderr;
		}
	} else {
		logging_fp = stderr;
	}

	if( ! prefix_disabled )
	{
		fprintf( logging_fp , "[%lu]\t%s: ", time( NULL ) , logging_value_to_name( loglevel_map, level ) );
	}

	va_start(ap, format);

	vfprintf( logging_fp, format, ap );

	va_end(ap);

	if( logging_fp && logging_fp != stderr )
	{
		fclose( logging_fp );
	}

logging_end:
	pthread_mutex_unlock( &logging_mutex );
}

void logging_init(void)
{
	char *name = NULL;

	pthread_mutex_init( &logging_mutex, NULL );

	pthread_mutex_lock( &logging_mutex );

	if( is_yes( config_get("logging.enabled") ) )
	{
		logging_threshold = logging_name_to_value( loglevel_map, config_get("logging.log_level") ) ;

		// Disable logging to a file if readonly option is set
		if( is_no( config_get("readonly") ) )
		{
			name = config_get("logging.log_file") ;
			if( name )
			{
				logging_file_name = strdup( name );
			} else {
				logging_file_name = NULL;
			}
		} else {
			logging_file_name = NULL;
		}
		
		
		logging_enabled = 1;
	}

	pthread_mutex_unlock( &logging_mutex );
}

void logging_stop(void)
{
	pthread_mutex_lock( &logging_mutex);

	if( logging_file_name )
	{
		free( logging_file_name );
		logging_file_name = NULL;
	}

	logging_enabled = 0;
	
	pthread_mutex_unlock( &logging_mutex );

	pthread_mutex_destroy( &logging_mutex );
}
