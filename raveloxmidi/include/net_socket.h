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

#ifdef HAVE_ALSA
#include "raveloxmidi_alsa.h"
#endif

#include "midi_state.h"

typedef enum raveloxmidi_socket_type_t {
	RAVELOXMIDI_SOCKET_FD_TYPE,
#ifdef HAVE_ALSA
	RAVELOXMIDI_SOCKET_ALSA_TYPE,
#endif
} raveloxmidi_socket_type_t;

typedef struct raveloxmidi_socket_t {
	int fd;
	raveloxmidi_socket_type_t type;
	char *packet;
	size_t packet_size;
	midi_state_t *state;
	pthread_mutex_t	lock;
#ifdef HAVE_ALSA
	snd_rawmidi_t	*handle;
#endif
} raveloxmidi_socket_t;

void net_socket_lock( raveloxmidi_socket_t *raveloxmidi_socket );
void net_socket_unlock( raveloxmidi_socket_t *raveloxmidi_socket );

void net_socket_send_lock( void );
void net_socket_send_unlock( void );
raveloxmidi_socket_t *net_socket_add( int new_socket );
int net_socket_create( int family, char *ip_address, unsigned int port );
int net_socket_listener_create( int family, char *ip_address, unsigned int port );
int net_socket_init( void );
int net_socket_teardown( void );

int net_socket_listener( int fd );
void net_socket_loop_init(void);
void net_socket_loop_teardown(void);
int net_socket_fd_loop(void);
void net_socket_loop_shutdown(int signal);
int net_socket_get_shutdown_status( void) ;

int net_socket_get_data_socket( void );
int net_socket_get_control_socket( void );
int net_socket_get_local_socket( void );

void net_socket_set_fds( void );
int net_socket_get_shutdown_fd( void );

int net_socket_read( int fd );

/* Indicate which socket should be which in the socket array */
#define NET_SOCKET_CONTROL_PORT 0
#define NET_SOCKET_DATA_PORT 1
#define NET_SOCKET_LOCAL_PORT 2

#define DEFAULT_BLOCK_SIZE 2048
#define NET_SOCKET_DEFAULT_RING_BUFFER	10240

#define OK		0
#define SHUTDOWN	1
#endif
