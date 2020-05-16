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

#ifndef KV_TABLE_H
#define KV_TABLE_H

typedef struct kv_item_t {
	char *key;
	char *value;
} kv_item_t;

typedef struct kv_table_t {
	char *name;
	size_t count;
	kv_item_t **items;
	pthread_mutex_t lock;
} kv_table_t;

kv_table_t *kv_table_create( char *name );
void kv_table_dump( kv_table_t *table );
void kv_table_destroy( kv_table_t **table );
kv_item_t *kv_find_item( kv_table_t *table, char *key );
char *kv_get_value( kv_table_t *table, char *key );
void kv_add_item( kv_table_t *table, char *key, char *value );
void kv_get_item_by_index( kv_table_t *table, size_t index, char **key, char **value );
size_t kv_item_count( kv_table_t *table );

void kv_table_lock( kv_table_t *table );
void kv_table_unlock( kv_table_t *table );

#endif
