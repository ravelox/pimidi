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

#ifndef NET_SOCKET_H
#define NET_SOCKET_H

#include "config.h"

void net_socket_add( int new_socket );
int net_socket_create( unsigned int port );
int net_socket_ipv6_create( unsigned int port );
int net_socket_init( void );
int net_socket_teardown( void );

int net_socket_listener( int fd );
void net_socket_loop_init(void);
void net_socket_loop_teardown(void);
int net_socket_fd_loop(void);
int net_socket_alsa_loop(void);
void net_socket_wait_for_alsa(void);
void net_socket_loop_shutdown(int signal);

void net_socket_set_fds( void );
extern uint8_t _max_ctx;

/* Indicate which socket should be the data port */
#define DATA_PORT 1

#define BUFFER_32K	32768

#endif
