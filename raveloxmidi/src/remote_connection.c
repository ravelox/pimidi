/*
   This file is part of raveloxmidi.

   Copyright (C) 2019 Dave Kelly

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
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>

#include <pthread.h>

#include <errno.h>
extern int errno;

#include "config.h"

#include "net_applemidi.h"
#include "net_response.h"
#include "net_socket.h"
#include "net_connection.h"

#include "applemidi_inv.h"
#include "applemidi_sync.h"
#include "applemidi_feedback.h"
#include "applemidi_by.h"

#include "dns_service_discover.h"

#include "utils.h"

#include "raveloxmidi_config.h"
#include "logging.h"

#include "remote_connection.h"

pthread_t sync_thread;

void remote_connect_init( void )
{
	dns_service_t *found_service = NULL;
	char *remote_service_name = NULL;
	char *client_name = NULL;
	net_response_t *response = NULL;
	net_ctx_t *ctx;
	int use_ipv4, use_ipv6;
	uint32_t initiator = 0, ssrc = 0;
	char *p1 = NULL;
	char *p2 = NULL;
	int remote_port_number = 0;


	if( ! config_is_set( "remote.connect" ) )
	{
		logging_printf(LOGGING_WARN, "remote_connect_init: Called with no remote.connect value\n");
		return;
	}

	remote_service_name = (char *) strdup( config_string_get("remote.connect") );

	logging_printf(LOGGING_DEBUG, "remote_connect_init: Looking for [%s]\n", remote_service_name);

	p1 = remote_service_name;
	p2 = p1 + strlen( remote_service_name );

	/* Work backwards to find a colon ':' */
	/* If a ']' character is found first, we'll assume this is going to be an direct connect address */

	while( p2 > p1 )
	{
		if( *p2 == ']' ) break;
		if( *p2 == ':' ) break;
		p2--;
	}



	/* If no colon ':' or ']' was found, we'll assume this is a service name to be located */
	if( p2 == p1 )
	{
		use_ipv4 = is_yes( config_string_get("service.ipv4") ) ;
		use_ipv6 = is_yes( config_string_get("service.ipv6") ) ;

		if( dns_discover_services( use_ipv4, use_ipv6 ) <= 0 )
		{
			logging_printf(LOGGING_WARN, "remote_connect_init: No services available\n");
			free( remote_service_name );
			return;
		}

		found_service = dns_discover_by_name( remote_service_name );
		goto make_remote_connection;
	}  else {
		/* If there is a colon ':', split the string to determine the port number */
		if( *p2 == ':' )
		{
			*p2='\0';
			p2++;
			remote_port_number = atoi( p2 );
			p2 = remote_service_name + strlen( remote_service_name ) - 1;
		}

		/* If there is a ']', work forwards from the start of the string to remove the '[' */
		if ( *p2 == ']' )
		{
			*p2='\0';
			while( p1 < p2 )
			{
				if( *p1 == '[' ) break;
				p1++;
			}

		}
		if( p1 == p2 )
		{
			p1 = remote_service_name;
		} else {
			*p1='\0';
			p1++;
		}

		if( remote_port_number == 0 )
		{
			logging_printf( LOGGING_ERROR, "remote_connect_init: No port number specified\n");
			free( remote_service_name );
			return;
		}

		logging_printf( LOGGING_DEBUG, "remote_connect_init: connect_string=>%s<\n", config_string_get("remote.connect") );
		logging_printf( LOGGING_DEBUG, "remote_connect_init: connect_address=>%s<, connect_port=%d\n", p1, remote_port_number );

		dns_discover_add( config_string_get("remote.connect"), p1, remote_port_number );
		found_service = dns_discover_by_name( config_string_get("remote.connect") );
	}
	

make_remote_connection:

	if( ! found_service )
	{
		logging_printf(LOGGING_WARN, "remote_connect_init: No service found: %s\n", remote_service_name );
		free( remote_service_name );
		return;
	}
	free( remote_service_name );

	logging_printf( LOGGING_DEBUG, "remote_connect_init: Found name=\"%s\" address=[%s]:%d\n", found_service->name, found_service->ip_address, found_service->port);
	ssrc = random_number();
	initiator = random_number();

	client_name = config_string_get("service.name");

	if( !client_name )
	{
		client_name = "RaveloxMIDIClient";
	}

	response = net_response_inv( ssrc, initiator, client_name );

	if( response )
	{
		ctx = net_ctx_register( ssrc, initiator, found_service->ip_address, found_service->port, found_service->name );

		if( ! ctx )
		{
			logging_printf( LOGGING_ERROR, "remote_connect_init: Unable to create socket context\n");
		} else {
			ctx->send_ssrc = ssrc;
			ctx->status = NET_CTX_STATUS_FIRST_INV;
			logging_printf( LOGGING_DEBUG, "remote_connect_init: Sending INV request to [%s]:%d\n", ctx->ip_address, ctx->control_port );
			net_ctx_send( ctx, response->buffer, response->len , USE_CONTROL_PORT );
		}
	}

	net_response_destroy( &response );
}

void remote_connect_teardown( void )
{
	net_applemidi_inv *by = NULL;
	net_response_t *response = NULL;
	net_applemidi_command *cmd = NULL;
	char *remote_service_name = NULL;
	net_ctx_t *ctx;

	remote_service_name = config_string_get("remote.connect");

	if( ! remote_service_name ) return;

	logging_printf( LOGGING_DEBUG, "remote_connect_teardown: Disconnecting from [%s]\n", remote_service_name);

	remote_connect_wait_for_thread();

	ctx = net_ctx_find_by_name( remote_service_name );
	
	if( ! ctx )
	{
		logging_printf( LOGGING_ERROR, "remote_connect_teardown: Unable to find connection for [%s]\n", remote_service_name);
		return;
	};

	// Build the BY packet
	by = net_applemidi_inv_create();
	
	if( ! by )
	{
		logging_printf( LOGGING_ERROR, "remote_connect_teardown: Unable to allocate memory for by packet\n");
		return;
	}

	by->ssrc = ctx->send_ssrc;
	by->version = 2;
	by->initiator = ctx->initiator;

	cmd = net_applemidi_cmd_create( NET_APPLEMIDI_CMD_END );
	
	if( ! cmd ) 
	{
		logging_printf( LOGGING_ERROR, "remote_connect_teardown: Unable to create AppleMIDI command\n");
		net_applemidi_inv_destroy( &by );
		goto remote_teardown_fail;
	}

	cmd->data = by;

	response = net_response_create();

	if( response )
	{
		int ret = 0;
		ret = net_applemidi_pack( cmd , &(response->buffer), &(response->len) );
		if( ret != 0 )
		{
			logging_printf( LOGGING_ERROR, "remote_connect_teardown: Unable to pack response to by command\n");
		} else {
			net_ctx_send( ctx, response->buffer, response->len , USE_CONTROL_PORT);
			hex_dump( response->buffer, response->len );
		}
	} else {
		logging_printf( LOGGING_ERROR, "remote_connect_teardown: Unable to create response packet\n");
	}

remote_teardown_fail:
	net_response_destroy( &response );
	net_applemidi_cmd_destroy( &cmd );
}

static void *remote_connect_sync_thread( void *data )
{
	net_response_t *response;
	net_ctx_t *ctx = NULL;
	char *remote_service_name = NULL;
	int shutdown_fd = 0;
	fd_set read_fds;
	struct timeval tv;
	int sync_interval = 0;
	int use_control = 0;

	logging_printf( LOGGING_DEBUG, "remote_connect_sync_thread: start\n");
	logging_printf( LOGGING_DEBUG, "rmeote_connect_sync_thread: sync.interval=%s\n", config_string_get("sync.interval"));

	shutdown_fd = net_socket_get_shutdown_fd();
	logging_printf( LOGGING_DEBUG, "remote_connect_sync_thread: shutdown_fd=%u\n", shutdown_fd );

	remote_service_name = config_string_get("remote.connect");
	sync_interval = config_int_get("sync.interval");

	/* Determine if CK messages are sent over the control port or not */
	/* This is a workaround for rtpMIDI not responding unless CK messages are sent via the data port */
	if( config_is_set("remote.use_control") )
	{
		use_control = ( is_yes( config_string_get("remote.use_control") ) ? 1 : 0 );
	} else {
		use_control = 1;
	}

	do
	{
		ctx = net_ctx_find_by_name( remote_service_name );
		if( ! ctx )
		{
			logging_printf( LOGGING_DEBUG, "remote_connect_sync_thread: No remote connection context\n");
			break;
		}
		FD_ZERO( &read_fds );
		FD_SET( shutdown_fd, &read_fds );
		memset( &tv, 0, sizeof( struct timeval ) );
		tv.tv_sec = sync_interval;
		select( shutdown_fd + 1, &read_fds, NULL , NULL, &tv );
		logging_printf( LOGGING_DEBUG, "remote_connect_sync_thread: select()=\"%s\"\n", strerror( errno ) );
		if( net_socket_get_shutdown_status() == SHUTDOWN )
		{
			logging_printf(LOGGING_DEBUG, "remote_connect_sync_thread: shutdown received during poll\n");
			break;
		}
		response = net_response_sync( ctx->send_ssrc , ctx->start );
		net_ctx_send( ctx, response->buffer, response->len, use_control );
		hex_dump( response->buffer, response->len );
		net_response_destroy( &response );
	} while( 1 );

	if( net_socket_get_shutdown_status() == SHUTDOWN )
	{
		logging_printf(LOGGING_DEBUG, "remote_connect_sync_thread: shutdown received\n");
	}
	logging_printf( LOGGING_DEBUG, "remote_connect_sync_thread: stop\n");

	return NULL;
}

void remote_connect_sync_start( void )
{
	logging_printf( LOGGING_DEBUG, "remote_connect_sync_start: Starting sync thread\n");
	pthread_create( &sync_thread, NULL, remote_connect_sync_thread, NULL );
}

void remote_connect_wait_for_thread( void )
{
	logging_printf( LOGGING_DEBUG, "remote_connect_wait_for_thread: Waiting\n");
	pthread_join( sync_thread, NULL );
	logging_printf( LOGGING_DEBUG, "remote_connect_wait_for_thread: Done\n");
}
