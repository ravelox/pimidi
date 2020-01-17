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

static unsigned int max_connections = 0;
static unsigned int num_connections = 0;
static net_ctx_t **connections = NULL;

static pthread_mutex_t connections_mutex = PTHREAD_MUTEX_INITIALIZER;

void net_connections_lock( void )
{
	pthread_mutex_lock( &connections_mutex );
}

void net_connections_unlock( void )
{
	pthread_mutex_unlock( &connections_mutex );
}

void net_ctx_lock( net_ctx_t *ctx )
{
	if( ! ctx ) return;
	pthread_mutex_lock( &(ctx->lock) );
}

void net_ctx_unlock( net_ctx_t *ctx )
{
	if( ! ctx ) return;
	pthread_mutex_unlock( &(ctx->lock) );
}

void net_ctx_destroy( net_ctx_t **ctx )
{
	if( ! ctx ) return;
	if( ! *ctx ) return;

	logging_printf(LOGGING_DEBUG,"net_ctx_destroy: ctx=%p\n", *ctx);
	if( (*ctx)->name ) FREENULL( "name",(void **)&((*ctx)->name) );
	if( (*ctx)->ip_address) FREENULL( "ip_address",(void **)&((*ctx)->ip_address) );
	journal_destroy( &((*ctx)->journal) );

	pthread_mutex_destroy( &((*ctx)->lock) );
	FREENULL( "net_ctx_destroy: ctx", (void**)ctx );
}

void net_ctx_dump( net_ctx_t *ctx )
{
	DEBUG_ONLY;
	if( ! ctx ) return;
	
	logging_printf( LOGGING_DEBUG, "net_ctx: ctx=%p, ssrc=0x%08x,status=[%s],send_ssrc=0x%08x,initiator=0x%08x,seq=%u,host=%s,control=%u,data=%u,start=%u\n",
		ctx, ctx->ssrc, net_ctx_status_to_string( ctx->status ), ctx->send_ssrc, ctx->initiator, ctx->seq, ctx->ip_address, ctx->control_port, ctx->data_port, ctx->start);
}

void net_ctx_dump_all( void )
{
	int i = 0;
	DEBUG_ONLY;
	logging_printf( LOGGING_DEBUG, "net_ctx_dump_all: start\n");

	for( i=0 ; i < num_connections; i++ )
	{
		net_ctx_dump( connections[i] );
	}
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
		free( ctx->ip_address );
	}
	ctx->ip_address = ( char *) strdup( ip_address );

	if( ctx->name )
	{
		free( ctx->name );
	}
	ctx->name = ( char *) strdup( name );

	ctx->status = NET_CTX_STATUS_IDLE;
	net_ctx_unlock( ctx );
}

net_ctx_t * net_ctx_find_unused( void )
{
	net_ctx_t *unused = NULL;
	int i = 0;

	/* If there are no connections at all, there can't be any unused ones */
	if( num_connections == 0 )  return NULL;

	net_connections_lock();
	/* Work through the current list to find an unused connection */
	for( i = 0 ; i < num_connections; i++ )
	{
		if( connections[i] )
		{
			if( connections[i]->status == NET_CTX_STATUS_UNUSED )
			{
				unused = connections[i];
				break;
			}
		}
	}
	net_connections_unlock();

	return unused; 
}

net_ctx_t * net_ctx_create( void )
{
	net_ctx_t *new_ctx = NULL;
	journal_t *journal = NULL;

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
	net_ctx_unlock( ctx );
}


void net_ctx_init( void )
{
	max_connections = config_int_get("network.max_connections");
	if( max_connections == 0 ) max_connections = 1;

	num_connections = 0;
	connections = NULL;

	pthread_mutex_init( &connections_mutex, NULL );
}

void net_ctx_teardown( void )
{
	int i = 0;

	net_connections_lock();
	for( i = num_connections; i > 0; i-- )
	{
		if( connections[i-1] )
		{
			net_ctx_destroy( &connections[i-1] );
			num_connections--;
		}
	}

	FREENULL("net_ctx_teardown:connections", (void **)&connections);
	net_connections_unlock();

	pthread_mutex_destroy( &connections_mutex );
}

net_ctx_t * net_ctx_find_by_ssrc( uint32_t ssrc)
{
	int i = 0;

	if( num_connections == 0 ) return NULL;
	if( ! connections ) return NULL;

	logging_printf( LOGGING_DEBUG, "net_ctx_find_by_ssrc: ssrc=0x%08x\n", ssrc );

	net_connections_lock();
	for( i = 0; i < num_connections; i++ )
	{
		if( connections[i] && (connections[i]->status != NET_CTX_STATUS_UNUSED ) )
		{
			if( connections[i]->ssrc == ssrc )
			{
				net_connections_unlock();
				return connections[i];
			}
		}
	}

	logging_printf( LOGGING_DEBUG, "net_ctx_find_by_ssrc: not found\n");

	net_connections_unlock();
	return NULL;
}

net_ctx_t * net_ctx_find_by_initiator( uint32_t initiator)
{
	int i = 0;

	if( num_connections == 0 ) return NULL;
	if( ! connections ) return NULL;

	logging_printf( LOGGING_DEBUG, "net_ctx_find_by_initiator: initiator=0x%08x\n", initiator );

	net_connections_lock();
	for( i = 0; i < num_connections; i++ )
	{
		if( connections[i] && (connections[i]->status != NET_CTX_STATUS_UNUSED ) )
		{
			if( connections[i]->initiator == initiator )
			{
				net_connections_unlock();
				return connections[i];
			}
		}
	}
	logging_printf( LOGGING_DEBUG, "net_ctx_find_by_initiator: not found\n");
	net_connections_unlock();
	return NULL;
}

net_ctx_t * net_ctx_find_by_name( char *name )
{
	int i = 0;

	if( num_connections == 0 ) return NULL;
	if( ! connections ) return NULL;

	if( ! name ) return NULL;

	logging_printf( LOGGING_DEBUG, "net_ctx_find_by_name: name=%s\n", name );

	net_connections_lock();
	for( i = 0; i < num_connections; i++ )
	{
		if( connections[i] && (connections[i]->status != NET_CTX_STATUS_UNUSED ) )
		{
			if( strcmp(connections[i]->name, name) == 0 )
			{
				net_connections_unlock();
				return connections[i];
			}
		}
	}
	logging_printf( LOGGING_DEBUG, "net_ctx_find_by_name: not found\n");

	net_connections_unlock();
	return NULL;
}
	
void net_ctx_add( net_ctx_t *ctx )
{
	net_ctx_t **new_list = NULL;

	if( ! ctx ) return;


	new_list = ( net_ctx_t ** ) realloc( connections, ( num_connections + 1 ) * sizeof( net_ctx_t * ) );

	if( ! new_list )
	{
		logging_printf( LOGGING_ERROR, "net_ctx_add: Insufficient memory to reallocate new connections list\n");
		return;
	}

	net_ctx_lock( ctx );
	ctx->status = NET_CTX_STATUS_IDLE;
	net_ctx_unlock( ctx );

	net_connections_lock();
	new_list[ num_connections ] = ctx;
	num_connections++;
	connections = new_list;
	net_connections_unlock();
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

	net_ctx_lock( ctx );

	/* Set up the destination address */
	memset((char *)&send_address, 0, sizeof( send_address));
	port_number = ( use_control == USE_CONTROL_PORT ? ctx->control_port : ctx->data_port );
	get_sock_info( ctx->ip_address, port_number, (struct sockaddr *)&send_address, &addr_len, &family);

	send_socket = ( use_control == USE_CONTROL_PORT ? net_socket_get_control_socket() : net_socket_get_data_socket() );
	socket_addr_len = sizeof( socket_address );
	getsockname(send_socket , (struct sockaddr *)&socket_address, &socket_addr_len);
	logging_printf( LOGGING_DEBUG, "net_ctx_send: outbound socket=%d\n", ntohs( ((struct sockaddr_in *)&socket_address)->sin_port ) );

	net_socket_socklist_lock();
	bytes_sent = sendto( send_socket, buffer, buffer_len , MSG_CONFIRM, (struct sockaddr *)&send_address, addr_len);
	net_socket_socklist_unlock();

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
	return num_connections;
}

int net_ctx_is_used( net_ctx_t *ctx )
{
	if( ! ctx ) return 0;
	return ctx->status != NET_CTX_STATUS_UNUSED;
}

net_ctx_t *net_ctx_find_by_index( int index )
{
	if( ! connections ) return NULL;
	if( num_connections == 0 ) return NULL;
	if( num_connections < index ) return NULL;

	return connections[ index ];
}
