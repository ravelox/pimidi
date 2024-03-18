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
#include "midi_state.h"
#include "net_connection.h"
#include "net_socket.h"
#include "rtp_packet.h"
#include "utils.h"

#include "dstring.h"

#include "raveloxmidi_config.h"
#include "logging.h"

#include "data_table.h"

static data_table_t *connections = NULL;

void net_connections_lock( void )
{
	data_table_lock( connections );
}

void net_connections_unlock( void )
{
	data_table_unlock( connections );
}

void net_ctx_lock( net_ctx_t *ctx )
{
	if( ! ctx ) return;
	X_MUTEX_LOCK( &(ctx->lock) );
}

void net_ctx_unlock( net_ctx_t *ctx )
{
	if( ! ctx ) return;
	X_MUTEX_UNLOCK( &(ctx->lock) );
}

void net_ctx_destroy( void **data )
{
	net_ctx_t **ctx = NULL;
	if( ! data ) return;
	if( ! *data ) return;

	ctx = (net_ctx_t **)data;
	logging_printf(LOGGING_DEBUG,"net_ctx_destroy: ctx=%p\n", *ctx);
	if( (*ctx)->name ) X_FREENULL( "name",(void **)&((*ctx)->name) );
	if( (*ctx)->ip_address) X_FREENULL( "ip_address",(void **)&((*ctx)->ip_address) );
	journal_destroy( &((*ctx)->journal) );

	if( (*ctx)->midi_state )
	{
		midi_state_destroy( &((*ctx)->midi_state) );
	}
	pthread_mutex_destroy( &((*ctx)->lock) );
	X_FREENULL( "net_ctx_destroy: ctx", data );
}

void net_ctx_dump( void *data )
{
	net_ctx_t *ctx = NULL;
	DEBUG_ONLY;
	if( ! data ) return;
	
	ctx = ( net_ctx_t *)data;
	logging_printf( LOGGING_DEBUG, "net_ctx: ctx=%p, ssrc=0x%08x,status=[%s],send_ssrc=0x%08x,initiator=0x%08x,seq=%u,host=%s,control=%u,data=%u,start=%u\n",
		ctx, ctx->ssrc, net_ctx_status_to_string( ctx->status ), ctx->send_ssrc, ctx->initiator, ctx->seq, ctx->ip_address, ctx->control_port, ctx->data_port, ctx->start);
	if( ctx->midi_state ) midi_state_dump( ctx->midi_state );
}

void net_ctx_dump_all( void )
{
	DEBUG_ONLY;
	logging_printf( LOGGING_DEBUG, "net_ctx_dump_all: start\n");

	data_table_dump( connections );

	logging_printf( LOGGING_DEBUG, "net_ctx_dump_all: end\n");
}

static void net_ctx_set( net_ctx_t *ctx, uint32_t ssrc, uint32_t initiator, uint32_t send_ssrc, uint16_t port, char *ip_address , char *name)
{
	if( ! ctx ) return;

	net_ctx_lock( ctx );
	ctx->ssrc = ssrc;
	ctx->send_ssrc = send_ssrc;
	ctx->initiator = initiator;
	ctx->control_port = port;
	ctx->data_port = port+1;
	ctx->start = time_in_microseconds();


	if( ctx->ip_address )
	{
		X_FREE( ctx->ip_address );
	}
	ctx->ip_address = ( char *) X_STRDUP( ip_address );

	if( ctx->name )
	{
		X_FREE( ctx->name );
	}
	ctx->name = ( char *) X_STRDUP( name );

	if( ctx->midi_state )
	{
		midi_state_reset( ctx->midi_state );
	}

	ctx->status = NET_CTX_STATUS_IDLE;
	net_ctx_unlock( ctx );
}

net_ctx_t * net_ctx_find_unused( void )
{
	net_ctx_t *current_ctx = NULL;
	size_t i = 0;
	size_t num_connections = 0;

	num_connections = data_table_item_count( connections );

	/* If there are no connections at all, there can't be any unused ones */
	if( num_connections == 0 )
	{
		logging_printf( LOGGING_DEBUG, "net_ctx_find_unused: zero items in connections list\n");
		return NULL;
	}

	net_connections_lock();

	/* Work through the current list to find an unused connection */
	for( i = 0 ; i < num_connections; i++ )
	{
		current_ctx = (net_ctx_t *) data_table_item_get( connections, i );
		logging_printf( LOGGING_DEBUG, "net_ctx_find_unused: index=%zu, ctx=%p\n", i, current_ctx );
		if( current_ctx )
		{
			logging_printf( LOGGING_DEBUG, "net_ctx_find_unused: index=%zu,status=[%s]\n", i, net_ctx_status_to_string( current_ctx->status ) );
			if( current_ctx->status == NET_CTX_STATUS_UNUSED )
			{
				break;
			}
		}
		current_ctx = NULL;
	}

	net_connections_unlock();

	return current_ctx; 
}

net_ctx_t * net_ctx_create( void )
{
	net_ctx_t *new_ctx = NULL;
	journal_t *journal = NULL;
	midi_state_t *new_midi_state = NULL;
	size_t ring_buffer_size = 0;

	new_ctx = ( net_ctx_t * ) X_MALLOC( sizeof( net_ctx_t ) );

	if( ! new_ctx )
	{
		logging_printf(LOGGING_ERROR,"net_ctx_create: Unable to allocate memory for new net_ctx_t\n");
		return NULL;
	}

	memset( new_ctx, 0, sizeof( net_ctx_t ) );
	new_ctx->seq = 1;

	journal_init( &journal );
	new_ctx->journal = journal;

	ring_buffer_size = config_int_get("read.ring_buffer_size");
	ring_buffer_size = MAX( NET_SOCKET_DEFAULT_RING_BUFFER, ring_buffer_size );
	new_midi_state = midi_state_create( ring_buffer_size );
	if( ! new_midi_state )
	{
		logging_printf( LOGGING_ERROR, "net_ctx_create: Unable to create midi_state_t for net_ctx_t\n");
	} else {
		new_ctx->midi_state = new_midi_state;
		logging_printf( LOGGING_DEBUG, "net_ctx_create: midi_state->ring=%p\n", new_midi_state->ring );
	}
	new_ctx->status = NET_CTX_STATUS_UNUSED;

	pthread_mutex_init( &new_ctx->lock , NULL);
	return new_ctx;

}

void net_ctx_reset( net_ctx_t *ctx )
{
	if( ! ctx ) return;

	logging_printf(LOGGING_DEBUG, "net_ctx_reset: ctx=%p\n", ctx );
	net_ctx_journal_reset( ctx );
	net_ctx_lock( ctx );
	ctx->seq = 1;
	ctx->status = NET_CTX_STATUS_UNUSED;

	if( ctx->midi_state )
	{
		midi_state_reset( ctx->midi_state );
	}

	net_ctx_unlock( ctx );
}


void net_ctx_init( void )
{
	connections = data_table_create( "connections", net_ctx_destroy, net_ctx_dump );
}

void net_ctx_teardown( void )
{
	data_table_destroy( &connections );
}

net_ctx_t * net_ctx_find_by_ssrc( uint32_t ssrc)
{
	size_t i = 0;
	size_t num_connections = 0;
	net_ctx_t *current_ctx = NULL;

	num_connections = data_table_item_count( connections );

	if( num_connections == 0 )
	{
		logging_printf( LOGGING_DEBUG, "net_ctx_find_by_ssrc: zero items in connections list\n");
		return NULL;
	}

	logging_printf( LOGGING_DEBUG, "net_ctx_find_by_ssrc: ssrc=0x%08x\n", ssrc );

	net_connections_lock();

	for( i = 0; i < num_connections; i++ )
	{
		current_ctx = (net_ctx_t *) data_table_item_get( connections, i );
		if( current_ctx && (current_ctx->status != NET_CTX_STATUS_UNUSED ) )
		{
			if( current_ctx->ssrc == ssrc )
			{
				break;
			}
		}
		current_ctx = NULL;
	}
	if( ! current_ctx )
	{
		logging_printf( LOGGING_DEBUG, "net_ctx_find_by_ssrc: not found\n");
	}

	net_connections_unlock();

	return current_ctx;
}

net_ctx_t * net_ctx_find_by_initiator( uint32_t initiator)
{
	size_t i = 0;
	size_t num_connections = 0;
	net_ctx_t *current_ctx = NULL;

	num_connections = data_table_item_count( connections );

	if( num_connections == 0 )
	{
		logging_printf( LOGGING_DEBUG, "net_ctx_find_by_initiator: zero items in connections list\n");
	  	return NULL;
	}

	logging_printf( LOGGING_DEBUG, "net_ctx_find_by_initiator: initiator=0x%08x\n", initiator );

	net_connections_lock();

	for( i = 0; i < num_connections; i++ )
	{
		current_ctx = (net_ctx_t *) data_table_item_get( connections, i );
		if( current_ctx && (current_ctx->status != NET_CTX_STATUS_UNUSED ) )
		{
			if( current_ctx->initiator == initiator )
			{
				break;
			}
		}
		current_ctx = NULL;
	}
	if( ! current_ctx )
	{
		logging_printf( LOGGING_DEBUG, "net_ctx_find_by_initiator: not found\n");
	}

	net_connections_unlock();

	return current_ctx;
}

net_ctx_t * net_ctx_find_by_name( char *name )
{
	size_t i = 0;
	size_t num_connections = 0;
	net_ctx_t *current_ctx = NULL;

	if( ! name ) return NULL;

	num_connections = data_table_item_count( connections );

	if( num_connections == 0 )
	{
		logging_printf( LOGGING_DEBUG, "net_find_by_name: zero items in connections list\n");
		return NULL;
	}

	logging_printf( LOGGING_DEBUG, "net_ctx_find_by_name: name=%s\n", name );

	net_connections_lock();

	for( i = 0; i < num_connections; i++ )
	{
		current_ctx = (net_ctx_t *)data_table_item_get( connections, i );
		if( current_ctx && (current_ctx->status != NET_CTX_STATUS_UNUSED ) )
		{
			if( strcmp(current_ctx->name, name) == 0 )
			{
				break;
			}
		}
		current_ctx = NULL;
	}

	if( ! current_ctx )
	{
		logging_printf( LOGGING_DEBUG, "net_ctx_find_by_name: not found\n");
	}

	net_connections_unlock();

	return current_ctx;
}
	
void net_ctx_add( net_ctx_t *ctx )
{
	net_ctx_lock( ctx );
	ctx->status = NET_CTX_STATUS_IDLE;
	net_ctx_unlock( ctx );

	data_table_add_item( connections, ctx );
}

net_ctx_t * net_ctx_register( uint32_t ssrc, uint32_t initiator, char *ip_address, uint16_t port, char *name )
{
	net_ctx_t *new_ctx = NULL;
	uint32_t send_ssrc = 0;
	uint8_t reuse = 0;

	logging_printf( LOGGING_DEBUG, "net_ctx_register: ssrc=0x%08x initiator=0x%08x ip_address=[%s] port=%u name=[%s]\n", ssrc, initiator, ip_address, port, name );

	/* Check to see if the ssrc already exists */
	new_ctx = net_ctx_find_by_ssrc( ssrc );

	if( ! new_ctx )
	{
		logging_printf( LOGGING_DEBUG, "net_ctx_register: Finding unused connection slot\n");

		/* Find an unused connection before trying to create a new one */
		new_ctx = net_ctx_find_unused();

		/* No unused slots available */
		if( ! new_ctx )	
		{
			new_ctx = net_ctx_create();

			/* Can't even create a new one */
			if( ! new_ctx )
			{
				logging_printf(LOGGING_ERROR, "net_ctx_register: Unable to create new net_ctx_t\n");
				return NULL;
			}
		} else {
			logging_printf(LOGGING_DEBUG, "net_ctx_register: Using existing unused slot\n");
			reuse = 1;
		}
	} else {
		logging_printf(LOGGING_WARN, "net_ctx_register: net_ctx: Attempt to register existing ssrc: 0x%08x\n", ssrc );
		goto register_end;
	}

	send_ssrc = random_number();
	net_ctx_set( new_ctx, ssrc, initiator, send_ssrc, port, ip_address , name);

	/* This is a new slot we need to create */
	if( reuse == 0)
	{
		net_ctx_add( new_ctx );
	}

register_end:
	net_ctx_dump_all();

	return new_ctx;
}

void net_ctx_add_journal_note( net_ctx_t *ctx, midi_note_t *midi_note )
{
	if( ! midi_note ) return;
	if( ! ctx) return;
	net_ctx_lock( ctx );
	midi_journal_add_note( ctx->journal, ctx->seq, midi_note );
	net_ctx_unlock( ctx );
	net_ctx_journal_dump( ctx );
}

void net_ctx_add_journal_control( net_ctx_t *ctx, midi_control_t *midi_control )
{
	if( ! midi_control ) return;
	if( !ctx ) return;
	net_ctx_lock( ctx );
	midi_journal_add_control( ctx->journal, ctx->seq, midi_control );
	net_ctx_unlock( ctx );
	net_ctx_journal_dump( ctx );
}

void net_ctx_add_journal_program( net_ctx_t *ctx, midi_program_t *midi_program )
{
	if( !midi_program ) return;
	if( !ctx ) return;
	net_ctx_lock( ctx );
	midi_journal_add_program( ctx->journal, ctx->seq, midi_program );
	net_ctx_unlock( ctx );
	net_ctx_journal_dump( ctx );
}

void net_ctx_journal_dump( net_ctx_t *ctx )
{
	if( ! ctx) return;

	net_ctx_lock( ctx );
	logging_printf( LOGGING_DEBUG, "net_ctx_journal_dump: has_data=%s\n", ( journal_has_data( ctx->journal ) ? "YES" : "NO" ) );
	if( ! journal_has_data( ctx->journal ) )
	{
		net_ctx_unlock( ctx );
		return;
	}
	journal_dump( ctx->journal );
	net_ctx_unlock( ctx );
}

void net_ctx_journal_pack( net_ctx_t *ctx, char **journal_buffer, size_t *journal_buffer_size)
{
	*journal_buffer = NULL;
	*journal_buffer_size = 0;

	if( ! ctx) return;

	net_ctx_lock( ctx );
	journal_pack( ctx->journal, journal_buffer, journal_buffer_size );
	net_ctx_unlock( ctx );
}
	
void net_ctx_journal_reset( net_ctx_t *ctx )
{
	if( ! ctx) return;

	net_ctx_lock( ctx );
	logging_printf(LOGGING_DEBUG,"net_ctx_journal_reset:ssrc=0x%08x\n", ctx->ssrc );
	journal_reset( ctx->journal);
	net_ctx_unlock( ctx );
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

	net_ctx_lock( ctx );
	ctx->seq += 1;
	net_ctx_unlock( ctx );
}

void net_ctx_send( net_ctx_t *ctx, unsigned char *buffer, size_t buffer_len , int use_control)
{
	struct sockaddr_in6 send_address;
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

	net_ctx_lock( ctx );

	/* Set up the destination address */
	memset((char *)&send_address, 0, sizeof( send_address));
	port_number = ( use_control == USE_CONTROL_PORT ? ctx->control_port : ctx->data_port );
	get_sock_info( ctx->ip_address, port_number, (struct sockaddr *)&send_address, &addr_len, &family);

	send_socket = ( use_control == USE_CONTROL_PORT ? net_socket_get_control_socket() : net_socket_get_data_socket() );

	bytes_sent = sendto( send_socket, buffer, buffer_len , MSG_DONTWAIT, (struct sockaddr *)&send_address, addr_len);

	if( bytes_sent < 0 )
	{
		logging_printf( LOGGING_ERROR, "net_ctx_send: Failed to send %u bytes to [%s]:%u\t%s\n", buffer_len, ctx->ip_address, port_number, strerror( errno ));
	} else {
		logging_printf( LOGGING_DEBUG, "net_ctx_send: write( bytes=%u,host=[%s]:%u, use_control=%d)\n", bytes_sent, ctx->ip_address, port_number , use_control );
	}

	net_ctx_unlock( ctx );
}

const char *net_ctx_status_to_string( net_ctx_status_t status )
{
	switch( status )
	{
		case NET_CTX_STATUS_IDLE: return "idle";
        	case NET_CTX_STATUS_FIRST_INV: return "first_inv";
        	case NET_CTX_STATUS_SECOND_INV: return "second_inv";
        	case NET_CTX_STATUS_REMOTE_CONNECTION: return "remote_connection";
		case NET_CTX_STATUS_UNUSED: return "unused";
		default: return "unknown";
	}
}

int net_ctx_get_num_connections( void )
{
	return data_table_item_count( connections );
}

int net_ctx_is_used( net_ctx_t *ctx )
{
	if( ! ctx ) return 0;
	return ctx->status != NET_CTX_STATUS_UNUSED;
}

net_ctx_t *net_ctx_find_by_index( int index )
{
	return (net_ctx_t *)data_table_item_get( connections, index );
}

char *net_ctx_connections_to_string( void )
{
	dstring_t *dstring = NULL;
	unsigned char *out_buffer = NULL;
	int i = 0;
	char ctx_buffer[1024];
	size_t connection_count = 0;
	size_t num_connections = 0;

	dstring = dstring_create( DSTRING_DEFAULT_BLOCK_SIZE );

	num_connections = data_table_item_count( connections );
	net_connections_lock();

	memset(ctx_buffer, 0, sizeof(ctx_buffer) );
	sprintf( ctx_buffer, "{\"connections\":[");
	dstring_append( dstring, ctx_buffer );

	for( i=0 ; i < num_connections; i++ )
	{
		net_ctx_t *ctx = NULL;

		ctx = data_table_item_get( connections, i );

		if( ctx->status == NET_CTX_STATUS_UNUSED ) continue;

		connection_count += 1;
		memset( ctx_buffer, 0, sizeof(ctx_buffer) );
		sprintf( ctx_buffer, "{\"id\":%d,\"name\":\"%s\",\"ctx\":\"%p\",\"ssrc\":\"0x%08x\",\"status\":\"%s\",\"send_ssrc\":\"0x%08x\",\"initiator\":\"0x%08x\",\"seq\":%u,\"host\":\"%s\",\"control\":%u,\"data\":%u,\"start\":%lu}",
			i, ( ctx->name ? ctx->name : "unknown"), ctx, ctx->ssrc, net_ctx_status_to_string( ctx->status ), ctx->send_ssrc, ctx->initiator, ctx->seq, ctx->ip_address, ctx->control_port, ctx->data_port, ctx->start);
		dstring_append( dstring, ctx_buffer );

		if( (i+1) < num_connections )
		{
			dstring_append(dstring, ",");
		}

	}
	dstring_append( dstring, "]" );

	memset( ctx_buffer, 0, sizeof(ctx_buffer ) );
	sprintf( ctx_buffer, ",\"count\":%zu}", connection_count );
	dstring_append( dstring, ctx_buffer );

	net_connections_unlock();

	out_buffer = X_STRDUP( dstring_value( dstring ) );

	logging_printf( LOGGING_DEBUG, "net_ctx_connections_to_string: out_buffer=[%s]\n", out_buffer);

	hex_dump( out_buffer, strlen( out_buffer));

	dstring_destroy( &dstring );

	return out_buffer;
}
