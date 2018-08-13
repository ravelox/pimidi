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
#include <getopt.h>

#include <errno.h>
extern int errno;

#include "config.h"

#include "raveloxmidi_config.h"
#include "logging.h"

static int num_items = 0;
static raveloxmidi_config_t **config_items = NULL;

static void config_set_defaults( void )
{
	config_add_item("network.bind_address", "0.0.0.0");
	config_add_item("network.control.port", "5004");
	config_add_item("network.data.port", "5005");
	config_add_item("network.local.port", "5006");
	config_add_item("network.socket_interval" , "5000" );
	config_add_item("network.max_connections", "8");
	config_add_item("service.name", "raveloxmidi");
	config_add_item("run_as_daemon", "yes");
	config_add_item("daemon.pid_file","raveloxmidi.pid");
	config_add_item("logging.enabled", "yes");
	config_add_item("logging.log_file", NULL);
	config_add_item("logging.log_level", "normal");
	config_add_item("security.check", "yes");
	config_add_item("readonly","no");
	config_add_item("inbound_midi","/dev/sequencer");
	config_add_item("file_mode", "0640");
}

static void config_load_file( char *filename )
{

	FILE *config_file = NULL;
	char config_line[ MAX_CONFIG_LINE + 1 ];
	char *p1,*p2;
	char *key, *value;

	if( ! filename ) return;

	config_file = fopen( filename, "r" );

	if( ! config_file )
	{
		fprintf( stderr, "Unable to open configuration file [%s]:%s\n", filename, strerror( errno ) );
		return;
	}

	while( 1 )
	{
		char *return_value = NULL;
		return_value = fgets( config_line , MAX_CONFIG_LINE, config_file );

		if( feof( config_file) ) break;

		if( ! return_value )
		{
			fprintf( stderr, "config_load_file: Unable to read config (file=\"%s\")\n", filename );
			break;
		}

		/* Remove any trailing white space */
		p2 = config_line + strlen( config_line ) - 1;
		while( *p2 && isspace( *p2 ) )
		{
			*p2 = '\0';
			p2--;
		}

		/* Remove leading white space */
		p1 = config_line;
		while( *p1 && isspace( *p1 ) ) p1++;

		/* Ignore any line that's marked as a comment */
		if( *p1 && *p1=='#' ) continue;

		key = p1;
		p2 = p1;

		/* Find the first whitespace character or '=' character */
		while( *p2 && ! isspace(*p2) && *p2 != '=' ) p2++;

		if( *p2 ) *p2++='\0';

		/* find the next non-whitespace and non '=' character */
		while( *p2 && ( isspace( *p2 ) || *p2=='=' ))  p2++;
		value = p2;

		config_add_item( key, value );
	}

	if( config_file) fclose( config_file );
}

void config_init( int argc, char *argv[] )
{
	static struct option long_options[] = {
		{"config",   required_argument, NULL, 'c'},
		{"debug",    no_argument, NULL, 'd'},
		{"info",    no_argument, NULL, 'I'},
		{"nodaemon", no_argument, NULL, 'N'},
		{"pidfile", required_argument, NULL, 'P'},
		{"readonly", no_argument, NULL, 'R'},
#ifdef HAVE_ALSA
		{"listinterfaces", no_argument, NULL, 'L'},
#endif
		{"help", no_argument, NULL, 'h'},
		{0,0,0,0}
	};
#ifdef HAVE_ALSA
	const char *short_options = "c:dIhNP:RL";
#else
	const char *short_options = "c:dIhNP:R";
#endif
	int c;
	config_items = NULL;

	config_set_defaults();

	while(1)
	{
		c = getopt_long( argc, argv, short_options, long_options, NULL);

		if( c == -1 ) break;

		switch(c) {
			/* If an argument is missing */
			case '?':
				exit(0);
			case 'c':
				config_add_item("config.file", optarg);
				break;
			case 'I':
				config_add_item("logging.enabled", "yes");
				config_add_item("logging.log_level", "info");
				break;
			case 'd':
				config_add_item("logging.enabled", "yes");
				config_add_item("logging.log_level", "debug");
				break;
			/* This option exits */
			case 'h':
				config_usage();
				exit(0);
			case 'N':
				config_add_item("run_as_daemon", "no");
				break;
			case 'P':
				config_add_item("daemon.pid_file", optarg);
				break;
			case 'R':
				config_add_item("readonly", "yes");
				break;
		}
	} 

	config_load_file( config_get("config.file") );

	if( strcasecmp( config_get("logging.log_level"), "debug" ) == 0 )
	{
		config_dump();
	}
}

void config_destroy( void )
{
	int i = 0;

	fprintf( stderr, "config_destroy config_items=%p num_items=%u\n", config_items, num_items - 1 );
	if( ! config_items ) return;
	if( num_items == 0 ) return;

	for( i = 0 ; i < num_items ; i++ )
	{
		if( config_items[i]->value )  free( config_items[i]->value );
		if( config_items[i]->key ) free( config_items[i]->key );
		if( config_items[i] ) free( config_items[i] );
	}

	if( config_items ) free( config_items );

	num_items = 0;
	config_items = NULL;
}

/* Public version */
char *config_get( char *key )
{
	int i = 0;
	if( num_items <= 0 ) return NULL;
	if( ! config_items ) return NULL;
	
	for( i=0 ; i < num_items ; i++ )
	{
		if( strcasecmp( key, config_items[i]->key ) == 0 )
		{
			return config_items[i]->value;
		}
	}
	return NULL;
}

static raveloxmidi_config_t *config_get_item( char *key )
{
	int i = 0 ;

	if( num_items <= 0 ) return NULL;
	if( ! config_items ) return NULL;

	for( i=0; i < num_items ; i ++ )
	{
		if( strcasecmp( key, config_items[i]->key ) == 0 )
		{
			return config_items[i];
		}
	}

	return NULL;
}

void config_add_item(char *key, char *value )
{
	raveloxmidi_config_t *new_item, *found_item;
	raveloxmidi_config_t **new_config_item_list = NULL;

	found_item = config_get_item( key );
	if( ! found_item )
	{
		new_item = ( raveloxmidi_config_t *)malloc( sizeof( raveloxmidi_config_t ));
		if( new_item )
		{
			new_item->key = (char *)strdup( key );
			if( value )
			{
				new_item->value = ( char *)strdup( value );
			} else {
				new_item->value = NULL;
			}

			new_config_item_list = (raveloxmidi_config_t **)realloc(config_items, sizeof( raveloxmidi_config_t * ) * (num_items + 1) );
			if( ! new_config_item_list )
			{
				fprintf(stderr, "config_add_item: Insufficient memory to create new config item\n");
				free( new_item );
				return;
			}
			config_items = new_config_item_list;
			config_items[ num_items ] = new_item;
			num_items++;
		}
	/* Overwrite any existing item that has the same key */
	} else {
		if( found_item->value ) free(found_item->value);
		if( value )
		{
			found_item->value = ( char *)strdup( value );
		} else {
			found_item->value = NULL;
		}
	}
}

void config_dump( void )
{
	int i = 0;
	if( ! config_items ) return;
	if( num_items == 0 ) return;

	for( i = 0 ; i < num_items; i++ )
	{
		fprintf( stderr, "%s = %s\n", config_items[i]->key, config_items[i]->value );
	}
}

void config_usage( void )
{
	fprintf( stderr, "Usage:\n");
	fprintf( stderr, "\traveloxmidi [-c filename] [-d] [-I] [-R] [-N] [-P filename] [-h]");
	fprintf( stderr, "\n");
	fprintf( stderr, "\traveloxmidi [--config filename] [--debug] [--info] [--readonly] [--nodaemon] [--pidfile filename] [--help]");
	fprintf( stderr, "\n");
	fprintf( stderr, "\n");
	fprintf( stderr, "-c filename\tName of config file to use\n");
	fprintf( stderr, "-d\t\tRun in debug mode\n");
	fprintf( stderr, "-d\t\tRun in debug mode\n");
	fprintf( stderr, "-N\t\tDo not run in the background\n");
	fprintf( stderr, "-P filename\tName of file to write background pid\n");
	fprintf( stderr, "-h\t\tThis output\n");
	fprintf( stderr, "\nThe following configuration file items are default:\n");
	config_dump();
}
