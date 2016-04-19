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

#ifndef NET_LISTENER_H
#define NET_LISTENER_H

typedef struct net_response_t {
	unsigned char *buffer;
	size_t len;
} net_response_t;

net_response_t * new_net_response( void );
void net_response_destroy( net_response_t **response );
int net_socket_create( unsigned int port );
int net_socket_destroy( void );
int net_socket_listener( void );
int net_socket_loop( unsigned int interval );
void net_socket_loop_shutdown(int signal);
int net_socket_setup( void );

extern uint8_t _max_ctx;

/* Indicate which socket should be the data port */
#define DATA_PORT 1
#endif
