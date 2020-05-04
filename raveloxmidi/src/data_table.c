/*
   This file is part of raveloxmidi.

   Copyright (C) 2020 Dave Kelly

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

#include "data_table.h"
#include "logging.h"
#include "utils.h"

void data_table_lock( data_table_t *table )
{
	if(! table ) return;
	pthread_mutex_lock( &(table->lock) );
}

void data_table_unlock( data_table_t *table )
{
	if(! table ) return;
	pthread_mutex_unlock( &(table->lock) );
}

void data_table_dump( data_table_t *table )
{
	size_t index = 0;
	size_t count = 0;
	DEBUG_ONLY;

	if( ! table ) return;

	data_table_lock( table );
	logging_printf( LOGGING_DEBUG, "data_table: name=[%s] count=[%u]\n", table->name, table->count );

	if( table->item_dump )
	{
		count = table->count;
		for( index = 0; index < count; index++ )
		{
			table->item_dump( table->items[ index ]->data );
		}
	}
	
	data_table_unlock( table );
}

data_table_t *data_table_create( char *name, data_item_destructor_t destructor , data_item_dump_t dump)
{
	data_table_t *new_table = NULL;
	
	new_table = ( data_table_t *) malloc( sizeof( data_table_t ) );

	if( ! new_table )
	{
		logging_printf( LOGGING_DEBUG, "data_table_create: Insufficient memory to create new table\n");
		return NULL;
	}

	memset( new_table, 0, sizeof( data_table_t ) );

	new_table->items = NULL;
	new_table->count = 0;
	pthread_mutex_init( &(new_table->lock), NULL );
	new_table->item_destructor = destructor;
	new_table->item_dump = dump;

	if( name )
	{
		new_table->name = (char *)strdup( name );
	}
	return new_table;
}

void data_table_destroy( data_table_t **table )
{
	if( ! *table ) return;


	data_table_lock( *table );

	logging_printf(LOGGING_DEBUG,"data_table_destroy: table=%p, name=[%s]\n", *table, ( (*table)->name ? (*table)->name : "" ) );

	if( (*table)->items )
	{
		size_t index = 0;

		for( index = 0 ; index < (*table)->count; index ++ )
		{
			if( ! (*table)->items[index] )
			{
				logging_printf( LOGGING_DEBUG, "data_table_destroy: index=%u undefined\n", index);
				continue;
			}

			if( (*table)->item_destructor )
			{
				if( (*table)->items[index]->data )
				{
					logging_printf( LOGGING_DEBUG, "data_table_destroy: calling item destructor : index=%u\n", index);
					(*table)->item_destructor( &( (*table)->items[index]->data ) );
				}
			}
			free( (*table)->items[index] );
			(*table)->items[index] = NULL;
		}
		free( (*table)->items );
		(*table)->items = NULL;
	}

	(*table)->count = 0;
	if( (*table)->name ) free( (*table)->name );

	data_table_unlock( *table );

	pthread_mutex_destroy( &((*table)->lock) );

	FREENULL("data_table", (void **)table );
}

void data_table_set_item_destructor( data_table_t *table, data_item_destructor_t destructor )
{
	if( ! table ) return;
	if( ! destructor ) return;

	data_table_lock( table );
	table->item_destructor = destructor;
	data_table_unlock( table );
}

int data_table_add_item( data_table_t *table, void *data )
{
	size_t i = 0;
	data_item_t *new_item = NULL;

	if(! table ) return 0;
	if(! data ) return 0;

	data_table_lock( table );

	logging_printf( LOGGING_DEBUG, "data_table_add_item: table=[%s] data=%p\n", table->name, data);

	/* Find an unused data item slot */
	if( table->items )
	{
		for( i = 0; i < table->count; i++ )
		{
			if( ! table->items[i] ) continue;
			if( table->items[i]->used == 1 ) continue;

			new_item = table->items[i];
			logging_printf( LOGGING_DEBUG, "data_table_item_add: unused found=%p\n", new_item);
			break;
		}
	}

	if( ! new_item )
	{
		data_item_t **new_items;

		/* Create a new item for the list */
		new_item = ( data_item_t * )malloc( sizeof( data_item_t ) );
		if( ! new_item )
		{
			logging_printf(LOGGING_ERROR,"data_table_add_item: Insufficient memory to create new table item\n");
			data_table_unlock( table );
			return 0;
		}
	
		/* Extend the list */
		new_items = ( data_item_t **)realloc( table->items, sizeof( data_item_t * ) * (table->count + 1) );
		if(! new_items )
		{
			logging_printf(LOGGING_ERROR,"data_table_add_item: Insufficient memory to extend table list\n");
			free(new_item);
			data_table_unlock( table );
			return 0;
		}

		table->items = new_items;
		table->items[ table->count ] = new_item;
		table->count++;

	}

	new_item->data = data;
	new_item->used = 1;

	data_table_unlock( table );

	return 1;
}

size_t data_table_item_count( data_table_t *table )
{
	size_t ret = 0;

	if( ! table ) return ret;
	
	data_table_lock( table );
	ret = table->count;
	data_table_unlock( table );
	return ret;
}

size_t data_table_unused_count( data_table_t *table )
{
	size_t i = 0;
	size_t ret = 0;

	if(! table ) return 0;

	data_table_lock( table );

	if(! table->items) goto data_table_unused_count_end;
	if(table->count == 0) goto data_table_unused_count_end;

	for( i = 0; i < table->count; i++ )
	{
		if( table->items[i] )
		{
			if( table->items[i]->used == 0 )
			{
				ret += 1;
			}
		}
	}
data_table_unused_count_end:
	data_table_unlock( table );

	return ret;
}

int data_table_item_is_unused( data_table_t *table, size_t index )
{
	int ret = 0;

	if( ! table ) return ret;
	if( ! table->items ) return ret;
	if( table->count == 0 ) return ret;
	if( index > table->count ) return ret;
	if( ! table->items[index] ) return ret;

	if( table->items[index]->used == 0 ) ret = 1;

	return ret;
}

/* Caller needs to lock the table first */
void *data_table_item_get( data_table_t *table, size_t index )
{
	void *ret = NULL;

	if( ! table ) return ret;

	if( ! table->items ) return NULL;
	if( table->count == 0 ) return NULL;
	if( index > table->count ) return NULL;
	if( ! table->items[ index ] ) return NULL;

	ret  = table->items[index]->data;

	return ret;
}

void data_table_delete_item( data_table_t *table, size_t index )
{
	if( ! table ) return;

	data_table_lock( table );

	if( ! table->items ) goto delete_item_end;
	if( table->count == 0 ) goto delete_item_end;
	if( index > table->count ) goto delete_item_end;
	if( ! table->items[ index ] ) goto delete_item_end;

	if( table->item_destructor )
	{
		data_table_lock( table );
		table->item_destructor( table->items[index]->data );
		free( &(table->items[index]->data) );
		table->items[index]->data = NULL;
		table->items[index]->used = 0;
	}

delete_item_end:
	data_table_unlock( table );
}

char *data_table_name( data_table_t *table )
{
	char *ret = NULL;

	if( ! table ) return ret;
	data_table_lock( table );
	ret = table->name;
	data_table_unlock( table );

	return ret;
}
