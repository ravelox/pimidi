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

#include <errno.h>
extern int errno;

#include "config.h"

#include "midi_journal.h"
#include "net_connection.h"
#include "rtp_packet.h"
#include "utils.h"

#include "raveloxmidi_config.h"
#include "logging.h"

static net_ctx_t *_ctx_head = NULL;
static unsigned int _max_ctx = 0;

static net_ctx_t *_iterator_current = NULL;

void net_ctx_destroy( net_ctx_t **ctx )
{
	net_ctx_t *next_ctx = NULL;
	net_ctx_t *prev_ctx = NULL;

	if( ! ctx ) return;
	if( ! *ctx ) return;

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

	FREENULL( "net_ctx_remove: ctx",(void**)ctx );
}

void net_ctx_dump( net_ctx_t *ctx )
{
	if( ! ctx ) return;
	
	logging_printf( LOGGING_DEBUG, "net_ctx: ssrc=0x%08x,send_ssrc=0x%08x,initiator=0x%08x,seq=0x%08x,host=%s,control=%u,data=%u\n",
		ctx->ssrc, ctx->send_ssrc, ctx->initiator, ctx->seq, ctx->ip_address, ctx->control_port, ctx->data_port);
}

static void net_ctx_set( net_ctx_t *ctx, uint32_t ssrc, uint32_t initiator, uint32_t send_ssrc, uint32_t seq, uint16_t port, char *ip_address )
{
	journal_t *journal = NULL;

	if( ! ctx ) return;

	ctx->ssrc = ssrc;
	ctx->send_ssrc = send_ssrc;
	ctx->initiator = initiator;
	ctx->seq = seq;
	ctx->control_port = port;
	ctx->start = time( NULL );

	if( ctx->ip_address )
	{
		free( ctx->ip_address );
	}
	ctx->ip_address = ( char *) strdup( ip_address );

	journal_init( &journal );
	ctx->journal = journal;
}

static net_ctx_t * net_ctx_create( void )
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
	new_ctx->seq = 0x0000;

	journal_init( &journal );

	new_ctx->journal = journal;
	new_ctx->next = NULL;
	new_ctx->prev = NULL;

	return new_ctx;

}

void net_ctx_init( void )
{
	_max_ctx = config_int_get("network.max_connections");
	if( _max_ctx == 0 ) _max_ctx = 1;

	_ctx_head = NULL;
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
}

net_ctx_t * net_ctx_find_by_ssrc( uint32_t ssrc)
{
	net_ctx_t *current_ctx = _ctx_head;

	while( current_ctx )
	{
		if( current_ctx->ssrc == ssrc )
		{
			return current_ctx;
		}
		current_ctx = current_ctx->next;
	}

	return current_ctx;
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
	
net_ctx_t * net_ctx_register( uint32_t ssrc, uint32_t initiator, char *ip_address, uint16_t port )
{
	net_ctx_t *current_ctx = NULL;
	net_ctx_t *last_ctx = NULL;
	net_ctx_t *new_ctx = NULL;
	time_t now = 0;
	unsigned int send_ssrc = 0;

	/* Check to see if the ssrc already exists */
	new_ctx = net_ctx_find_by_ssrc( ssrc );
	if( ! new_ctx )
	{
		new_ctx = net_ctx_create();
	}

	if( ! new_ctx )
	{
		logging_printf(LOGGING_ERROR, "net_ctx_register: Unable to create new net_ctx_t\n");
		return NULL;
	}


	last_ctx = net_ctx_get_last();
	if( ! _ctx_head ) _ctx_head = new_ctx;

	now = time(NULL);
	send_ssrc = rand_r( (unsigned int *)&now );

	net_ctx_set( new_ctx, ssrc, initiator, send_ssrc, 0x638F, port, ip_address );
	new_ctx->prev = last_ctx;

	if( last_ctx ) last_ctx->next = new_ctx;

	for( net_ctx_iter_start_head(); net_ctx_iter_has_next(); net_ctx_iter_next() )
	{
		current_ctx = net_ctx_iter_current();
		net_ctx_dump( current_ctx );
	}

	return new_ctx;
}

void net_ctx_add_journal_note( net_ctx_t *ctx, midi_note_t *midi_note )
{
	if( ! midi_note ) return;
	if( ! ctx) return;
	midi_journal_add_note( ctx->journal, ctx->seq, midi_note );
	journal_dump( ctx->journal );
}

void net_ctx_add_journal_control( net_ctx_t *ctx, midi_control_t *midi_control )
{
	if( ! midi_control ) return;
	if( !ctx ) return;
	midi_journal_add_control( ctx->journal, ctx->seq, midi_control );
	journal_dump( ctx->journal );
}

void net_ctx_journal_dump( net_ctx_t *ctx )
{
	if( ! ctx) return;

	logging_printf( LOGGING_DEBUG, "net_ctx_journal_dump: Journal has data: %s\n", ( journal_has_data( ctx->journal ) ? "YES" : "NO" ) );
	if( ! journal_has_data( ctx->journal ) ) return;
	journal_dump( ctx->journal );
}

void net_ctx_journal_pack( net_ctx_t *ctx, char **journal_buffer, size_t *journal_buffer_size)
{
	*journal_buffer = NULL;
	*journal_buffer_size = 0;

	if( ! ctx) return;

	journal_pack( ctx->journal, journal_buffer, journal_buffer_size );
}
	
void net_ctx_journal_reset( net_ctx_t *ctx )
{
	if( ! ctx) return;

	logging_printf(LOGGING_DEBUG,"net_ctx_journal_reset:ssrc=0x%08x\n", ctx->ssrc );
	journal_reset( ctx->journal);
}

void net_ctx_update_rtp_fields( net_ctx_t *ctx, rtp_packet_t *rtp_packet)
{
	if( ! rtp_packet ) return;
	if( ! ctx ) return;

	rtp_packet->header.seq = ctx->seq;
	rtp_packet->header.timestamp = time(0) - ctx->start;
	rtp_packet->header.ssrc = ctx->send_ssrc;
}

void net_ctx_increment_seq( net_ctx_t *ctx )
{
	if( ! ctx ) return;

	ctx->seq += 1;
}

void net_ctx_send( int send_socket, net_ctx_t *ctx, unsigned char *buffer, size_t buffer_len )
{
	struct sockaddr_in send_address;
	ssize_t bytes_sent = 0;
	struct timeval start;

	logging_printf( LOGGING_ERROR, "net_ctx_send: socket=%d, ctx ptr=%p, buffer ptr=%p, buffer_len=%u\n", send_socket, ctx, buffer, buffer_len);

	if( ! buffer ) return;
	if( buffer_len <= 0 ) return;
	if( ! ctx ) return;

	net_ctx_dump( ctx );
	net_ctx_journal_dump( ctx );

	/* Set up the destination address */
	memset((char *)&send_address, 0, sizeof( send_address));
	send_address.sin_family = AF_INET;
	send_address.sin_port = htons( ctx->data_port ) ;
	inet_aton( ctx->ip_address, &send_address.sin_addr );

	gettimeofday(&start, NULL);
	bytes_sent = sendto( send_socket, buffer, buffer_len , 0 , (struct sockaddr *)&send_address, sizeof( send_address ) );

	if( bytes_sent < 0 )
	{
		logging_printf( LOGGING_ERROR, "net_ctx_send: Failed to send %u bytes to %s:%u\n%s\n", buffer_len, ctx->ip_address, ctx->data_port , strerror( errno ));
	} else {
		profile_duration("net_ctx_send:", start.tv_usec);
		logging_printf( LOGGING_DEBUG, "net_ctx_send: write( bytes=%u,host=%s,port=%u)\n", bytes_sent, ctx->ip_address, ctx->data_port );
	}
}

net_ctx_t *net_ctx_iter_start_head(void)
{
	_iterator_current = _ctx_head;
	logging_printf(LOGGING_DEBUG,"net_ctx_iter_start_head: current=%p\n", _iterator_current );
	
}
net_ctx_t *net_ctx_iter_start_tail(void)
{
	_iterator_current = net_ctx_get_last();
	logging_printf(LOGGING_DEBUG,"net_ctx_iter_start_end: current=%p\n", _iterator_current );
	
}

net_ctx_t *net_ctx_iter_current(void)
{
	logging_printf(LOGGING_DEBUG,"net_ctx_iter_current: current=%p\n", _iterator_current );
	return _iterator_current;
}

int net_ctx_iter_has_current( void )
{
	logging_printf(LOGGING_DEBUG,"net_ctx_iter_has_current: current=%p\n", _iterator_current );
	return ( _iterator_current ? 1 : 0 );
}

int net_ctx_iter_has_next(void)
{
	logging_printf(LOGGING_DEBUG,"net_ctx_iter_has_next: current=%p\n", _iterator_current );
	if(! _iterator_current ) return 0;
	return (_iterator_current->next ? 1 : 0);
}

int net_ctx_iter_has_prev(void)
{
	logging_printf(LOGGING_DEBUG,"net_ctx_iter_has_prev: current=%p\n", _iterator_current );
	if(! _iterator_current ) return 0;
	return (_iterator_current->prev ? 1 : 0);
}

void net_ctx_iter_next(void)
{
	logging_printf(LOGGING_DEBUG,"net_ctx_iter_next: current=%p\n", _iterator_current );
	if(_iterator_current)
	{
		_iterator_current = _iterator_current->next;
	}
}

void net_ctx_iter_prev(void)
{
	logging_printf(LOGGING_DEBUG,"net_ctx_iter_prev: current=%p\n", _iterator_current );
	if(_iterator_current)
	{
		_iterator_current = _iterator_current->prev;
	}
}
