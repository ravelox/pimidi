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

#ifndef DATA_TABLE_H
#define DATA_TABLE_H

#include <unistd.h>
#include <pthread.h>


typedef struct data_item_t {
	void *data;
	int used;
} data_item_t;

typedef void (*data_item_destructor_t)(void ** );
typedef void (*data_item_dump_t)(void *);

typedef struct data_table_t {
	char *name;
	data_item_t **items;
	size_t count;
	pthread_mutex_t lock;
	data_item_destructor_t item_destructor;
	data_item_dump_t item_dump;
} data_table_t;


void data_table_lock( data_table_t *table );
void data_table_unlock( data_table_t *table );
void date_table_dump( data_table_t *table );
void date_item_dump( data_item_t *item );
data_table_t *data_table_create( char *name, data_item_destructor_t destructor, data_item_dump_t dump );
void data_table_destroy( data_table_t **table );
void data_table_set_item_destructor( data_table_t *table, data_item_destructor_t destructor );
void data_table_set_item_dump( data_table_t *table, data_item_dump_t dump);
int data_table_add_item( data_table_t *table, void *data );
size_t data_table_item_count( data_table_t *table );
size_t data_table_unused_count( data_table_t *table );
int data_table_item_is_unused( data_table_t *table, size_t index );
void *data_table_item_get( data_table_t *table, size_t index );
void data_table_delete_item( data_table_t *table, size_t index );
char *data_table_name( data_table_t *table );

#endif
