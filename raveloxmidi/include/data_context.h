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

#ifndef DATA_CONTEXT_H
#define DATA_CONTEXT_H

#include <stdint.h>
#include <pthread.h>

typedef void (*data_context_destroy_func_t)(void *context_data );

typedef struct data_context_t {
	void *data;
	uint32_t reference;
	pthread_mutex_t lock;
	data_context_destroy_func_t destroy_func;
} data_context_t;

data_context_t *data_context_create( data_context_destroy_func_t func);
void data_context_destroy( data_context_t **context );

void data_context_acquire( data_context_t *context );
void data_context_release( data_context_t **context );

#endif
