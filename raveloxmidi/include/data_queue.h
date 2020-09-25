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

#ifndef DATA_QUEUE_H
#define DATA_QUEUE_H

typedef void (*data_queue_action_func_t)(void *item, void *context );

typedef struct data_queue_item_t {
	void *data;
	void *context;
	struct data_queue_item_t *next;
} data_queue_item_t;

typedef struct data_queue_t {
        char *name;
        data_queue_item_t *top;
	data_queue_item_t *bottom;
	unsigned char state;
	unsigned char shutdown;
        pthread_mutex_t lock;
        data_queue_action_func_t action;
	pthread_t queue_thread;
	unsigned long counter;
	pthread_cond_t data_signal;
} data_queue_t;


/* Public functions */
data_queue_t * data_queue_create( char *name, data_queue_action_func_t action );
void data_queue_destroy( data_queue_t **queue );
void data_queue_add( data_queue_t *queue, void *data, void *context);
void data_queue_stop( data_queue_t *queue );
void data_queue_join( data_queue_t *queue );
void data_queue_start( data_queue_t *queue );

#define DATA_QUEUE_SHUTDOWN 1
#define DATA_QUEUE_CONTINUE 0

#define DATA_QUEUE_EMPTY	0
#define DATA_QUEUE_USED		1
#endif
