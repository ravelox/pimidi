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

#ifndef _RAVELOXMIDI_CONFIG_H
#define _RAVELOXMIDI_CONFIG_H

#include "kv_table.h"

int config_init( int argc, char *argv[]);
void config_teardown( void );

char *config_string_get( char *key );
int config_int_get( char *key );
long config_long_get( char *key );
int config_is_set( char *key );

void config_add_item(char *key, char *value);
void config_dump( void );

void config_usage( void );

typedef struct raveloxmidi_config_iter_t {
	char *prefix;
	int index;
} raveloxmidi_config_iter_t;

raveloxmidi_config_iter_t *config_iter_create( char *prefix );
void config_iter_destroy( raveloxmidi_config_iter_t **iter );
void config_iter_reset( raveloxmidi_config_iter_t *iter );
void config_iter_next( raveloxmidi_config_iter_t *iter  );
char *config_iter_string_get( raveloxmidi_config_iter_t *iter );
int config_iter_int_get( raveloxmidi_config_iter_t *iter );
long config_iter_long_get( raveloxmidi_config_iter_t *iter );
int config_iter_is_set( raveloxmidi_config_iter_t *iter );

#define MAX_CONFIG_LINE	1024

#define MAX_ITER_INDEX	10000

#define CONFIG_DUMP		1
#define CONFIG_DUMP_EXIT	2
#endif
