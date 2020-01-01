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

static kv_table_t *config_items = NULL;

static void config_set_defaults( void )
{
	config_add_item("network.control.port", "5004");
	config_add_item("network.data.port", "5005");
	config_add_item("network.local.port", "5006");
	config_add_item("network.socket_timeout" , "30" );
	config_add_item("network.max_connections", "8");
	config_add_item("service.name", "raveloxmidi");
	config_add_item("service.ipv4", "yes");
	config_add_item("service.ipv6", "no");
	config_add_item("run_as_daemon", "no");
	config_add_item("daemon.pid_file","raveloxmidi.pid");
	config_add_item("logging.enabled", "yes");
	config_add_item("logging.log_file", NULL );
	config_add_item("logging.log_level", "normal");
	config_add_item("security.check", "yes");
	config_add_item("readonly","no");
	config_add_item("inbound_midi","/dev/sequencer");
	config_add_item("file_mode", "0640");
	config_add_item("discover.timeout","5");
	config_add_item("sync.interval","10");
	config_add_item("network.read.blocksize","2048");
#ifdef HAVE_ALSA
	config_add_item("alsa.input_buffer_size", "4096" );
#endif

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

		if( feof( config_file) && ! return_value ) break;

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
	int dump_config = 0;
	static struct option long_options[] = {
		{"config",   required_argument, NULL, 'c'},
		{"debug",    no_argument, NULL, 'd'},
		{"info",    no_argument, NULL, 'i'},
		{"nodaemon", no_argument, NULL, 'N'},
		{"pidfile", required_argument, NULL, 'P'},
		{"readonly", no_argument, NULL, 'R'},
		{"dumpconfig", no_argument, NULL, 'C'},
		{"version", no_argument, NULL, 'v'},
		{"help", no_argument, NULL, 'h'},
		{0,0,0,0}
	};
#ifdef HAVE_ALSA
	const char *short_options = "c:dIhNP:RCv";
#else
	const char *short_options = "c:dIhNP:RCv";
#endif
	int c;

	config_items = kv_table_create("config_items");
	if( ! config_items )
	{
		fprintf(stderr,"Unable to initialise config table\n");
		exit(1);
	}
		
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
			case 'i':
				config_add_item("logging.enabled", "yes");
				config_add_item("logging.log_level", "info");
				break;
			case 'd':
				config_add_item("logging.enabled", "yes");
				config_add_item("logging.log_level", "debug");
				dump_config = 1;
				break;
			/* This option exits */
			case 'h':
				config_usage();
				exit(0);
			/* This option exits */
			case 'v':
				fprintf(stderr, "%s (%s)\n", PACKAGE, VERSION);
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
			case 'C':
				dump_config = 1;
				break;
		}
	} 

	config_load_file( config_string_get("config.file") );

	if( dump_config == 1 )
	{
		config_dump();
	}
}

void config_teardown( void )
{
	if( ! config_items )
	{
		fprintf( stderr, "NO CONFIG ITEMS\n");
		return;
	}

	logging_printf( LOGGING_DEBUG, "config_teardown config_items=%p count=%lu\n", config_items, config_items->count );

	kv_table_reset( config_items );

	free( config_items );
	config_items = NULL;
}

/* Public version */
char *config_string_get( char *key )
{
	return kv_get_value( config_items, key );
}

int config_int_get( char *key )
{
	char *item_string = NULL;

	item_string = config_string_get( key );
	if(! item_string ) return 0;
	if( strlen( item_string ) == 0 ) return 0;
	return atoi( item_string );
}
	
long config_long_get( char *key )
{
	char *item_string = NULL;

	item_string = config_string_get( key );
	if( ! item_string ) return 0;
	if( strlen( item_string ) == 0 ) return 0;
	return atol( item_string );
}

int config_is_set( char *key )
{
	kv_item_t *item = NULL;

	item  = kv_find_item( config_items, key );
	return ( ( item != NULL ) && ( item->value != NULL ) && ( strlen(item->value) > 0 ) );
}

raveloxmidi_config_iter_t *config_iter_create( char *prefix )
{
	raveloxmidi_config_iter_t *new_iter = NULL;
	if( ! prefix ) return NULL;
	new_iter = ( raveloxmidi_config_iter_t * ) malloc( sizeof( raveloxmidi_config_iter_t ) );
	if( ! new_iter ) return NULL;
	new_iter->prefix = ( char * ) strdup( prefix );
	new_iter->index = 0;

	return new_iter;
}

void config_iter_destroy( raveloxmidi_config_iter_t **iter )
{
	if( ! iter ) return;
	if( ! *iter ) return;

	if( (*iter)->prefix )
	{
		free( (*iter)->prefix );
		(*iter)->prefix = NULL;
	}

	free( *iter );
	*iter = NULL;
}

void config_iter_reset( raveloxmidi_config_iter_t *iter )
{
	if( ! iter ) return;
	iter->index = 0;
}

void config_iter_next( raveloxmidi_config_iter_t *iter )
{
	if( ! iter ) return;
	if( iter->index < MAX_ITER_INDEX ) iter->index += 1;
}

static char *config_make_key( char *prefix , int index )
{
	size_t key_len = 0;
	char *key = NULL;
	if( ! prefix ) return NULL;
	key_len = strlen( prefix ) + 7;
	key = ( char * ) malloc( key_len );
	if( ! key ) return NULL;
	sprintf( key, "%s.%d", prefix, index );
	return key;
}

char *config_iter_string_get( raveloxmidi_config_iter_t *iter )
{
	char *key = NULL;
	char *result = NULL;
	if( ! iter ) return NULL;
	if( ! iter->prefix ) return NULL;
	key = config_make_key( iter->prefix, iter->index );
	if( ! key ) return NULL;
	result = config_string_get( key );
	free( key );
	return result;
}

int config_iter_int_get( raveloxmidi_config_iter_t *iter )
{
	char *key = NULL;
	int result = 0;
	if( ! iter ) return 0;
	if( ! iter->prefix ) return 0;
	key = config_make_key( iter->prefix, iter->index );
	if( ! key ) return 0;
	result = config_int_get( key );
	free( key );
	return result;
}

long config_iter_long_get( raveloxmidi_config_iter_t *iter )
{
	char *key = NULL;
	long result = 0;
	if( ! iter ) return 0;
	if( ! iter->prefix ) return 0;
	key = config_make_key( iter->prefix, iter->index );
	if( ! key ) return 0;
	result = config_long_get( key );
	free( key );
	return result;
}

int config_iter_is_set( raveloxmidi_config_iter_t *iter )
{
	char *key = NULL;
	int result = 0;
	if( ! iter ) return 0;
	if( ! iter->prefix ) return 0;
	key = config_make_key( iter->prefix , iter->index);
	if( ! key ) return 0;
	result = config_is_set( key );
	free( key );
	return result;
}

void config_add_item(char *key, char *value )
{
	kv_add_item( config_items, key, value );
}

void config_dump( void )
{
	if( ! config_items ) return;
	if( config_items->count <= 0 ) return;

	kv_table_dump( config_items );
}

void config_usage( void )
{
	fprintf( stderr, "Usage:\n");
	fprintf( stderr, "\traveloxmidi [-c filename] [-d] [-i] [-R] [-N] [-P filename] [-C] [-D] [-h]");
	fprintf( stderr, "\n");
	fprintf( stderr, "\traveloxmidi [--config filename] [--debug] [--info] [--readonly] [--nodaemon] [--pidfile filename] [--dumpconfig] [--discover] [--help]");
	fprintf( stderr, "\n");
	fprintf( stderr, "\n");
	fprintf( stderr, "-c filename\tName of config file to use\n");
	fprintf( stderr, "-d\t\tRun in debug mode\n");
	fprintf( stderr, "-i\t\tRun in info mode\n");
	fprintf( stderr, "-N\t\tDo not run in the background\n");
	fprintf( stderr, "-P filename\tName of file to write background pid\n");
	fprintf( stderr, "-C\t\tDump the current config to stderr\n");
	fprintf( stderr, "-D\t\tDiscover rtpMIDI services\n");
	fprintf( stderr, "-h\t\tThis output\n");
	fprintf( stderr, "\nThe following configuration file items are default:\n");
	config_dump();
}
