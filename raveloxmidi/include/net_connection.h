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

#ifndef NET_CONNECTION_H
#define NET_CONNECTION_H

#include "midi_note.h"
#include "midi_control.h"
#include "rtp_packet.h"
#include "midi_journal.h"

// Maximum number of connection entries in the connection table
#define MAX_CTX 8

#define USE_DATA_PORT	0
#define USE_CONTROL_PORT	1

typedef enum net_ctx_status_t {
	NET_CTX_STATUS_IDLE,
	NET_CTX_STATUS_FIRST_INV,
	NET_CTX_STATUS_SECOND_INV,
	NET_CTX_STATUS_REMOTE_CONNECTION,
	NET_CTX_STATUS_UNUSED,
} net_ctx_status_t;

typedef struct net_ctx_t {
	net_ctx_status_t	status;
	uint32_t	ssrc;
	uint32_t	send_ssrc;
	uint32_t	initiator;
	uint32_t	seq;
	uint16_t	control_port;
	uint16_t	data_port;
	long		start;
	char * 		ip_address;
	char *		name;
	journal_t	*journal;
	pthread_mutex_t	lock;
} net_ctx_t;

net_ctx_t *net_ctx_create( void );
void net_ctx_destroy( net_ctx_t **ctx );
void net_ctx_dump( net_ctx_t *ctx );
void net_ctx_dump_all( void );
void net_ctx_lock( net_ctx_t *ctx );
void net_ctx_unlock( net_ctx_t *ctx );
void net_connections_lock( void );
void net_connections_unlock( void );
void net_ctx_init( void );
void net_ctx_teardown( void );
net_ctx_t * net_ctx_find_by_id( uint8_t id );
net_ctx_t * net_ctx_find_by_ssrc( uint32_t ssrc);
net_ctx_t * net_ctx_find_by_initiator( uint32_t initiator);
net_ctx_t * net_ctx_find_by_name( char *name );
net_ctx_t * net_ctx_register( uint32_t ssrc, uint32_t initiator, char *ip_address, uint16_t port , char *name);
char *net_ctx_status_to_string( net_ctx_status_t status );

void net_ctx_add_journal_note( net_ctx_t *ctx, midi_note_t *midi_note );
void net_ctx_add_journal_control( net_ctx_t *ctx, midi_control_t *midi_control );
void net_ctx_add_journal_program( net_ctx_t *ctx, midi_program_t *midi_program );

void net_ctx_journal_dump( net_ctx_t *ctx);
void net_ctx_journal_pack( net_ctx_t *ctx, char **journal_buffer, size_t *journal_buffer_size);
void net_ctx_journal_reset( net_ctx_t *ctx );
void net_ctx_update_rtp_fields( net_ctx_t *ctx, rtp_packet_t *rtp_packet);
void net_ctx_send( net_ctx_t *ctx, unsigned char *buffer, size_t buffer_len , int use_control );
void net_ctx_increment_seq( net_ctx_t *ctx );

net_ctx_t *net_ctx_find_by_index( int index );
int net_ctx_is_used( net_ctx_t *ctx );
int net_ctx_get_num_connections( void );

#endif
