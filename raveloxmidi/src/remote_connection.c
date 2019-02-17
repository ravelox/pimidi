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

static int remote_connected = 0;
static int ssrc = 0;
static int initiator = 0;

void remote_connect_init( void )
{
	dns_service_t *found_service = NULL;
	char *remote_service_name = NULL;
	char *client_name = NULL;
	net_applemidi_inv *inv = NULL;
	net_response_t *response = NULL;
	net_applemidi_command *cmd = NULL;
	net_ctx_t *ctx;

	remote_service_name = config_string_get("remote.connect");

	if( (! remote_service_name)  || ( strlen( remote_service_name ) <=0 ) )
	{
		logging_printf(LOGGING_WARN, "remote_connect_init: remote.connect option is present but not set\n");
		return;
	}

	logging_printf(LOGGING_DEBUG, "remote_connect_init: Looking for [%s]\n", remote_service_name);

	remote_connected = 0;
	if( dns_discover_services() <= 0 )
	{
		logging_printf(LOGGING_WARN, "remote_connect_init: No services available\n");
		return;
	}


	found_service = dns_discover_by_name( remote_service_name );

	if( ! found_service )
	{
		logging_printf(LOGGING_WARN, "remote_connect_init: No service found: %s\n", remote_service_name );
		return;
	}

	ssrc = random_number();
	initiator = random_number();

	// Build the INV packet
	inv = net_applemidi_inv_create();
	
	if( ! inv )
	{
		logging_printf( LOGGING_ERROR, "remote_connect_init: Unable to allocate memory for inv packet\n");
		return;
	}

	inv->ssrc = ssrc;
	inv->version = 2;
	inv->initiator = initiator;
	client_name = config_string_get("client.name");
	if( client_name )
	{
		inv->name = (char *)strdup( client_name );
	} else {
		inv->name = (char *)strdup( "RaveloxMIDIClient" );
	}

	cmd = net_applemidi_cmd_create( NET_APPLEMIDI_CMD_INV );
	
	if( ! cmd ) 
	{
		logging_printf( LOGGING_ERROR, "remote_connect_init: Unable to create AppleMIDI command\n");
		net_applemidi_inv_destroy( &inv );
		goto remote_connect_fail;
	}

	cmd->data = inv;

	response = net_response_create();

	if( response )
	{
		int ret = 0;
		ret = net_applemidi_pack( cmd , &(response->buffer), &(response->len) );
		if( ret != 0 )
		{
			logging_printf( LOGGING_ERROR, "remote_connect_init: Unable to pack response to inv command\n");
		} else {
			ctx = net_ctx_create();
			ctx->ip_address = ( char * ) strdup( found_service->ip_address );
			ctx->data_port = found_service->port;

			logging_printf( LOGGING_DEBUG, "remote_connect_init: Creating connection context for %s:%u\n", ctx->ip_address, ctx->data_port );

			if( ! ctx )
			{
				logging_printf( LOGGING_ERROR, "remote_connect_init: Unable to create socket context\n");
			} else {
				net_ctx_send( net_socket_get_data_socket() , ctx, response->buffer, response->len );
				net_ctx_send( net_socket_get_control_socket() , ctx, response->buffer, response->len );
			}

			net_ctx_destroy( &ctx );
		}
	} else {
		logging_printf( LOGGING_ERROR, "remote_connect_init: Unable to create response packet\n");
	}

remote_connect_fail:
	net_response_destroy( &response );
	net_applemidi_cmd_destroy( &cmd );
}

void remote_connect_teardown( void )
{
	if( ! remote_connected ) return;
}

