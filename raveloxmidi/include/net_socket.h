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

void net_socket_lock( void );
void net_socket_unlock( void );
void net_socket_add( int new_socket );
int net_socket_create( int family, char *ip_address, unsigned int port );
int net_socket_listener_create( int family, char *ip_address, unsigned int port );
int net_socket_init( void );
int net_socket_teardown( void );

int net_socket_listener( int fd );
void net_socket_loop_init(void);
void net_socket_loop_teardown(void);
int net_socket_fd_loop(void);
int net_socket_alsa_loop(void);
void net_socket_wait_for_alsa(void);
void net_socket_loop_shutdown(int signal);
int net_socket_get_shutdown_status( void) ;

int net_socket_get_data_socket( void );
int net_socket_get_control_socket( void );
int net_socket_get_local_socket( void );

void net_socket_set_fds( void );
int net_socket_get_shutdown_fd( void );

/* Indicate which socket should be which in the socket array */
#define NET_SOCKET_CONTROL_PORT 0
#define NET_SOCKET_DATA_PORT 1
#define NET_SOCKET_LOCAL_PORT 2

#define DEFAULT_BLOCK_SIZE 2048

#define OK		0
#define SHUTDOWN	1
#endif
