/*
   This file is part of raveloxmidi.

   Copyright (C) 2019 Dave Kelly

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
#include <string.h>

#include "kv_table.h"
#include "logging.h"

kv_table_t *kv_table_create( char *name )
{
	kv_table_t *new_table = NULL;

	new_table = ( kv_table_t *)malloc( sizeof( kv_table_t ) );
	if( ! new_table ) 
	{
		return NULL;
	}

	if( name ) new_table->name = strdup( name );

	new_table->items = NULL;
	new_table->count = 0;

	return new_table;
}

void kv_table_dump( kv_table_t *table )
{
	int i = 0;

	if( ! table ) return;
	if( ! table->items ) return;

	if( table->name )
	{
		logging_printf( LOGGING_DEBUG, "kv_table: name=[%s]\n", table->name );
	}

	for( i=0; i < table->count; i++ )
	{
		if( table->items[i]->key )
		{
			if( table->items[i]->value )
			{
				logging_printf( LOGGING_DEBUG, "\t[%s] = [%s]\n", table->items[i]->key, table->items[i]->value);
			}
		}
	}
}

void kv_table_reset( kv_table_t *table )
{
	int i = 0;

	if( ! table ) return;
	if( ! table->items) return;
	if( table->count <= 0 ) return;
	
	for(i=0; i < table->count; i++)
	{
		if( table->items[i] )
		{
			if( table->items[i]->key )
			{
				free( table->items[i]->key );
				table->items[i]->key = NULL;
			}
			if( table->items[i]->value )
			{
				free( table->items[i]->value );
				table->items[i]->value = NULL;
			}
			free( table->items[i] );
			table->items[i] = NULL;
		}
	}

	free( table->items);
	table->items = NULL;

	table->count = 0;
	if( table->name ) free( table->name);
	table->name = NULL;
}
		
kv_item_t *kv_find_item( kv_table_t *table, char *key )
{
	int i = 0;

	if( ! table ) return NULL;
	if( ! key ) return NULL;
	if( ! table->items ) return NULL;
	if( table->count <= 0 ) return NULL;

	for( i = 0; i < table->count; i++ )
	{
		if( strcasecmp( key, table->items[i]->key ) == 0 )
		{
			return table->items[i];
		}
	}
	
	return NULL;
}

char *kv_get_value( kv_table_t *table, char *key )
{
	kv_item_t *item = NULL;

	item = kv_find_item( table, key );
	if( ! item ) return NULL;

	return item->value;
}

void kv_add_item( kv_table_t *table, char *key, char *value )
{
	kv_item_t *new_item = NULL;
	kv_item_t **new_item_list = NULL;

	if( ! table ) return;
	if( ! key ) return;
	
	if( strlen( key ) == 0 ) return;

	new_item = kv_find_item( table, key );

	if( ! new_item )
	{
		new_item = (kv_item_t *)malloc( sizeof( kv_item_t ) );
		if( ! new_item )
		{
			return;
		}
		memset( new_item, 0, sizeof( kv_item_t ) );
		new_item->key = (char *)strdup( key );
		if( value )
		{
			new_item->value = (char *)strdup( value );
		} else {
			new_item->value = NULL;
		}

		new_item_list = ( kv_item_t **)realloc( table->items, sizeof( kv_item_t * ) * ( table->count + 1 ) );
		if( ! new_item_list )
		{
			if( new_item->value ) free( new_item->value);
			if( new_item->key ) free( new_item->key );
			free( new_item );
			return;
		}

		table->items = new_item_list;
		table->items[ table->count ] = new_item;
		table->count += 1;
	} else {
		if( new_item->value ) free( new_item->value );
		if( value )
		{
			new_item->value = ( char * )strdup( value );
		} else {
			new_item->value = NULL;
		}
	}
}
