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

#include "raveloxmidi_config.h"

static int num_items = 0;
static raveloxmidi_config_t **config_items = NULL;

static void config_load_file( char *filename )
{

	FILE *config_file = NULL;

	if( ! filename ) return;

	config_file = fopen( filename, "r" );

	if( ! config_file )
	{
		fprintf( stderr, "Unable to open configuration file [%s]:%s\n", filename, strerror( errno ) );
		return;
	}

	while( 1 )
	{
	}

	if( config_file) fclose( config_file );
}

void config_init( int argc, char *argv[] )
{
	static struct option long_options[] = {
		{"config",   required_argument, NULL, 'c'},
		{"debug",    required_argument, NULL, 'd'},
		{"nodaemon", required_argument, NULL, 'N'},
		{0,0,0,0}
	};
	char c;
	uint8_t 

	num_items = 0 ;
	config_items = NULL;

	while(1)
	{
		c = getopt_long( argc, argv, "c:d:N", long_options, NULL);

		if( c == -1 ) break;

		switch(c) {
			case 'c':
				config_add_item("config.file", optarg);
				break;
			case 'd':
				config_add_item("debug", NULL);
				break;
			case 'N':
				config_add_item("nodaemon", NULL);
				break;
		}
	} 

	config_load_file( config_get("config.file") );
}

void config_destroy( void )
{
	int i = 0;

	if( ! config_items ) return;
	if( num_items == 0 ) return;

	for( i = 0 ; i < num_items ; i++ )
	{
		if( config_items[i]->value ) free( config_items[i]->value );
		if( config_items[i]->key ) free( config_items[i]->key );
		if( config_items[i] ) free( config_items[i] );
	}

	if( config_items ) free( config_items );
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
		if( strcasecmp( key, config_items[i]->key ) )
		{
			return config_items[i];
		}
	}

	return NULL;
}

void config_add_item(char *key, char *value )
{
	raveloxmidi_config_t *new_item, *found_item;

	fprintf( stderr, "Adding key=%s\tvalue=%s\n", key, value );
	found_item = config_get_item( key );
	if( ! found_item )
	{
		new_item = ( raveloxmidi_config_t *)malloc( sizeof( raveloxmidi_config_t ));
		if( new_item )
		{
			new_item->key = (char *)strdup( key );
			new_item->value = ( char *)strdup( value );

			config_items = (raveloxmidi_config_t **)malloc(sizeof( raveloxmidi_config_t * ) * (num_items + 1) );
			if( config_items )
			{
				config_items[ num_items ] = new_item;
				num_items++;
			}
		}
	} else {
		if( found_item->value ) free(found_item->value);
		found_item->value = ( char *)strdup( value );
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

