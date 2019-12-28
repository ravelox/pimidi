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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>

#include <arpa/inet.h>

#include <pthread.h>

#include <errno.h>
extern int errno;

#include "config.h"

#include "midi_journal.h"
#include "net_connection.h"
#include "net_socket.h"
#include "rtp_packet.h"
#include "utils.h"

#include "raveloxmidi_config.h"
#include "logging.h"

static net_ctx_t *_ctx_head = NULL;
static unsigned int _max_ctx = 0;
static net_ctx_t *_iterator_current = NULL;

static pthread_mutex_t ctx_mutex;

void net_ctx_lock( void )
{
	pthread_mutex_lock( &ctx_mutex );
}

void net_ctx_unlock( void )
{
	pthread_mutex_unlock( &ctx_mutex );
}

void net_ctx_destroy( net_ctx_t **ctx )
{
	net_ctx_t *next_ctx = NULL;
	net_ctx_t *prev_ctx = NULL;

	if( ! ctx ) return;
	if( ! *ctx ) return;

	net_ctx_lock();

	FREENULL( "name",(void **)&((*ctx)->name) );
	FREENULL( "ip_address",(void **)&((*ctx)->ip_address) );
	journal_destroy( &((*ctx)->journal) );

	next_ctx = (*ctx)->next;
	prev_ctx = (*ctx)->prev;

	if( (*ctx)->prev )
	{
		(*ctx)->prev->next = next_ctx;
	}

	if( next_ctx )
	{
		next_ctx->prev = prev_ctx;
	} 
		
	if( ! next_ctx && ! prev_ctx )
	{
		_ctx_head = NULL;
	}

	if( ! prev_ctx )
	{
		_ctx_head = next_ctx;
	}

	FREENULL( "net_ctx_destroy: ctx", (void**)ctx );
	net_ctx_unlock();
}

void net_ctx_dump( net_ctx_t *ctx )
{
	DEBUG_ONLY;
	if( ! ctx ) return;
	
	logging_printf( LOGGING_DEBUG, "net_ctx: ssrc=0x%08x,send_ssrc=0x%08x,initiator=0x%08x,seq=%u,host=%s,control=%u,data=%u start=%u\n",
		ctx->ssrc, ctx->send_ssrc, ctx->initiator, ctx->seq, ctx->ip_address, ctx->control_port, ctx->data_port, ctx->start);
}

void net_ctx_dump_all( void )
{
	DEBUG_ONLY;
	logging_printf( LOGGING_DEBUG, "net_ctx_dump_all: start\n");
	net_ctx_lock();
	for( net_ctx_iter_start_head() ; net_ctx_iter_has_current(); net_ctx_iter_next())
	{
		net_ctx_dump( net_ctx_iter_current() );
	}
	net_ctx_unlock();
	logging_printf( LOGGING_DEBUG, "net_ctx_dump_all: end\n");
}

static void net_ctx_set( net_ctx_t *ctx, uint32_t ssrc, uint32_t initiator, uint32_t send_ssrc, uint16_t port, char *ip_address , char *name)
{
	if( ! ctx ) return;

	ctx->ssrc = ssrc;
	ctx->send_ssrc = send_ssrc;
	ctx->initiator = initiator;
	ctx->control_port = port;
	ctx->data_port = port+1;
	ctx->start = time_in_microseconds();

	if( ctx->ip_address )
	{
		free( ctx->ip_address );
	}
	ctx->ip_address = ( char *) strdup( ip_address );
	ctx->name = ( char *) strdup( name );

	logging_printf( LOGGING_DEBUG, "net_ctx_set\n");
	net_ctx_dump( ctx );
}

net_ctx_t * net_ctx_create( void )
{
	net_ctx_t *new_ctx;
	journal_t *journal;

	new_ctx = ( net_ctx_t * ) malloc( sizeof( net_ctx_t ) );

	if( ! new_ctx )
	{
		logging_printf(LOGGING_ERROR,"net_ctx_create: Unable to allocate memory for new net_ctx_t\n");
		return NULL;
	}

	memset( new_ctx, 0, sizeof( net_ctx_t ) );
	new_ctx->seq = 1;

	journal_init( &journal );

	new_ctx->journal = journal;
	new_ctx->next = NULL;
	new_ctx->prev = NULL;

	new_ctx->status = NET_CTX_STATUS_IDLE;

	return new_ctx;

}

void net_ctx_init( void )
{
	_max_ctx = config_int_get("network.max_connections");
	if( _max_ctx == 0 ) _max_ctx = 1;

	_ctx_head = NULL;

	pthread_mutex_init( &ctx_mutex, NULL );
}

void net_ctx_teardown( void )
{
	net_ctx_t *current_ctx = NULL;
	net_ctx_t *prev_ctx = NULL;

	current_ctx = net_ctx_get_last();

	while( current_ctx )
	{
		prev_ctx = current_ctx->prev;
		net_ctx_destroy( &current_ctx );
		current_ctx = prev_ctx;
	}

	pthread_mutex_destroy( &ctx_mutex );
}

net_ctx_t * net_ctx_find_by_ssrc( uint32_t ssrc)
{
	net_ctx_t *current_ctx = NULL;

	logging_printf( LOGGING_DEBUG, "net_ctx_find_by_ssrc: ssrc=0x%08x\n", ssrc );
	net_ctx_lock();
	for( net_ctx_iter_start_head() ; net_ctx_iter_has_current(); net_ctx_iter_next())
	{
		current_ctx = net_ctx_iter_current();
		if( current_ctx )
		{
			if( current_ctx->ssrc == ssrc )
			{
				net_ctx_unlock();
				return current_ctx;
			}
		}
	}
	net_ctx_unlock();
	logging_printf( LOGGING_DEBUG, "net_ctx_find_by_ssrc: not found\n");
	return NULL;
}

net_ctx_t * net_ctx_find_by_initiator( uint32_t initiator)
{
	net_ctx_t *current_ctx = NULL;

	logging_printf( LOGGING_DEBUG, "net_ctx_find_by_initiator: initiator=0x%08x\n", initiator );

	net_ctx_lock();
	for( net_ctx_iter_start_head() ; net_ctx_iter_has_current(); net_ctx_iter_next())
	{
		current_ctx = net_ctx_iter_current();
		if( current_ctx )
		{
			if( current_ctx->initiator == initiator )
			{
				net_ctx_unlock();
				return current_ctx;
			}
		}
	}

	net_ctx_unlock();
	logging_printf( LOGGING_DEBUG, "net_ctx_find_by_initiator: not found\n");
	return NULL;
}

net_ctx_t * net_ctx_find_by_name( char *name )
{
	net_ctx_t *current_ctx = NULL;
	
	if( ! name ) return NULL;

	logging_printf( LOGGING_DEBUG, "net_ctx_find_by_name: name=[%s]\n", name);
	net_ctx_lock();
	for( net_ctx_iter_start_head() ; net_ctx_iter_has_current(); net_ctx_iter_next() )
	{
		current_ctx = net_ctx_iter_current();
		if( current_ctx )
		{
			if( strcmp( current_ctx->name, name ) == 0 )
			{
				net_ctx_unlock();
				logging_printf( LOGGING_DEBUG, "net_ctx_find_by_name: found\n");
				return current_ctx;
			}
		}
	}

	net_ctx_unlock();
	logging_printf( LOGGING_DEBUG, "net_ctx_find_by_name: not found\n");
	return NULL;
}

net_ctx_t * net_ctx_get_last( void )
{
	net_ctx_t *current_ctx = _ctx_head;

	if( ! current_ctx ) return NULL;
	
	while(current_ctx->next)
	{
		current_ctx=current_ctx->next;
	}

	return current_ctx;
}
	
net_ctx_t * net_ctx_register( uint32_t ssrc, uint32_t initiator, char *ip_address, uint16_t port, char *name )
{
	net_ctx_t *last_ctx = NULL;
	net_ctx_t *new_ctx = NULL;
	uint32_t send_ssrc = 0;

	logging_printf( LOGGING_DEBUG, "net_ctx_register: ssrc=0x%08x initiator=0x%08x ip_address=[%s] port=%u name=[%s]\n", ssrc, initiator, ip_address, port, name );

	/* Check to see if the ssrc already exists */
	new_ctx = net_ctx_find_by_ssrc( ssrc );

	if( ! new_ctx )
	{
		new_ctx = net_ctx_create();
	} else {
		logging_printf(LOGGING_WARN, "net_ctx_register: net_ctx already exists\n");
		goto register_end;
	}

	if( ! new_ctx )
	{
		logging_printf(LOGGING_ERROR, "net_ctx_register: Unable to create new net_ctx_t\n");
		return NULL;
	}

	send_ssrc = random_number();
	net_ctx_set( new_ctx, ssrc, initiator, send_ssrc, port, ip_address , name);

	/* Add the connection to the list */
	if( ! _ctx_head )
	{
		_ctx_head = new_ctx;
	} else {
		last_ctx = net_ctx_get_last();
		if( last_ctx )
		{
			last_ctx->next = new_ctx;
		}
		new_ctx->prev = last_ctx;
	}
register_end:
	net_ctx_dump_all();

	return new_ctx;
}

void net_ctx_add_journal_note( net_ctx_t *ctx, midi_note_t *midi_note )
{
	if( ! midi_note ) return;
	if( ! ctx) return;
	net_ctx_lock();
	midi_journal_add_note( ctx->journal, ctx->seq, midi_note );
	net_ctx_unlock();
	net_ctx_journal_dump( ctx );
}

void net_ctx_add_journal_control( net_ctx_t *ctx, midi_control_t *midi_control )
{
	if( ! midi_control ) return;
	if( !ctx ) return;
	net_ctx_lock();
	midi_journal_add_control( ctx->journal, ctx->seq, midi_control );
	net_ctx_unlock();
	net_ctx_journal_dump( ctx );
}

void net_ctx_add_journal_program( net_ctx_t *ctx, midi_program_t *midi_program )
{
	if( !midi_program ) return;
	if( !ctx ) return;
	net_ctx_lock();
	midi_journal_add_program( ctx->journal, ctx->seq, midi_program );
	net_ctx_unlock();
	net_ctx_journal_dump( ctx );
}

void net_ctx_journal_dump( net_ctx_t *ctx )
{
	if( ! ctx) return;

	net_ctx_lock();
	logging_printf( LOGGING_DEBUG, "net_ctx_journal_dump: has_data=%s\n", ( journal_has_data( ctx->journal ) ? "YES" : "NO" ) );
	if( ! journal_has_data( ctx->journal ) )
	{
		net_ctx_unlock();
		return;
	}
	journal_dump( ctx->journal );
	net_ctx_unlock();
}

void net_ctx_journal_pack( net_ctx_t *ctx, char **journal_buffer, size_t *journal_buffer_size)
{
	*journal_buffer = NULL;
	*journal_buffer_size = 0;

	if( ! ctx) return;

	net_ctx_lock();
	journal_pack( ctx->journal, journal_buffer, journal_buffer_size );
	net_ctx_unlock();
}
	
void net_ctx_journal_reset( net_ctx_t *ctx )
{
	if( ! ctx) return;

	net_ctx_lock();
	logging_printf(LOGGING_DEBUG,"net_ctx_journal_reset:ssrc=0x%08x\n", ctx->ssrc );
	journal_reset( ctx->journal);
	net_ctx_unlock();
}

void net_ctx_update_rtp_fields( net_ctx_t *ctx, rtp_packet_t *rtp_packet)
{
	if( ! rtp_packet ) return;
	if( ! ctx ) return;

	rtp_packet->header.seq = ctx->seq;
	rtp_packet->header.timestamp = time_in_microseconds() - ctx->start;
	rtp_packet->header.ssrc = ctx->send_ssrc;
}

void net_ctx_increment_seq( net_ctx_t *ctx )
{
	if( ! ctx ) return;

	ctx->seq += 1;
}

void net_ctx_send( net_ctx_t *ctx, unsigned char *buffer, size_t buffer_len , int use_control)
{
	struct sockaddr_in6 send_address, socket_address;
	ssize_t bytes_sent = 0;
	socklen_t addr_len = 0, socket_addr_len = 0;
	int port_number = 0;
	int family = AF_UNSPEC;
	int send_socket = 0;

	if( ! buffer ) return;
	if( buffer_len <= 0 ) return;
	if( ! ctx ) return;

	net_ctx_dump( ctx );
	net_ctx_journal_dump( ctx );

	/* Set up the destination address */
	memset((char *)&send_address, 0, sizeof( send_address));
	port_number = ( use_control == USE_CONTROL_PORT ? ctx->control_port : ctx->data_port );
	get_sock_info( ctx->ip_address, port_number, (struct sockaddr *)&send_address, &addr_len, &family);

	send_socket = ( use_control == USE_CONTROL_PORT ? net_socket_get_control_socket() : net_socket_get_data_socket() );
	socket_addr_len = sizeof( socket_address );
	getsockname(send_socket , (struct sockaddr *)&socket_address, &socket_addr_len);
	logging_printf( LOGGING_DEBUG, "net_ctx_send: outbound socket=%d\n", ntohs( ((struct sockaddr_in *)&socket_address)->sin_port ) );

	net_socket_lock();
	bytes_sent = sendto( send_socket, buffer, buffer_len , MSG_CONFIRM, (struct sockaddr *)&send_address, addr_len);
	net_socket_unlock();

	if( bytes_sent < 0 )
	{
		logging_printf( LOGGING_ERROR, "net_ctx_send: Failed to send %u bytes to [%s]:%u\t%s\n", buffer_len, ctx->ip_address, port_number, strerror( errno ));
	} else {
		logging_printf( LOGGING_DEBUG, "net_ctx_send: write( bytes=%u,host=[%s]:%u, use_control=%d)\n", bytes_sent, ctx->ip_address, port_number , use_control );
	}
}

void net_ctx_iter_start_head(void)
{
	_iterator_current = _ctx_head;
	
}
void net_ctx_iter_start_tail(void)
{
	_iterator_current = net_ctx_get_last();
	
}

net_ctx_t *net_ctx_iter_current(void)
{
	return _iterator_current;
}

int net_ctx_iter_has_current( void )
{
	return ( _iterator_current ? 1 : 0 );
}

int net_ctx_iter_has_next(void)
{
	if(! _iterator_current ) return 0;
	return (_iterator_current->next ? 1 : 0);
}

int net_ctx_iter_has_prev(void)
{
	if(! _iterator_current ) return 0;
	return (_iterator_current->prev ? 1 : 0);
}

void net_ctx_iter_next(void)
{
	if(_iterator_current)
	{
		_iterator_current = _iterator_current->next;
	}
}

void net_ctx_iter_prev(void)
{
	if(_iterator_current)
	{
		_iterator_current = _iterator_current->prev;
	}
}

char *net_ctx_status_to_string( net_ctx_status_t status )
{
	switch( status )
	{
		case NET_CTX_STATUS_IDLE: return "idle";
        	case NET_CTX_STATUS_FIRST_INV: return "first_inv";
        	case NET_CTX_STATUS_SECOND_INV: return "second_inv";
        	case NET_CTX_STATUS_REMOTE_CONNECTION: return "remote_connection";
		default: return "unknown";
	}
}
