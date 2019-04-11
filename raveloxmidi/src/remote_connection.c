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
static uint32_t remote_connect_ssrc = 0;
static uint32_t remote_initiator = 0;

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

	logging_printf( LOGGING_DEBUG, "remote_connect_init: Found name=\"%s\" address=[%s]:%d\n", found_service->name, found_service->ip_address, found_service->port);
	remote_connect_ssrc = random_number();
	remote_initiator = random_number();

	// Build the INV packet
	inv = net_applemidi_inv_create();
	
	if( ! inv )
	{
		logging_printf( LOGGING_ERROR, "remote_connect_init: Unable to allocate memory for inv packet\n");
		return;
	}

	inv->ssrc = remote_connect_ssrc;
	inv->version = 2;
	inv->initiator = remote_initiator;
	client_name = config_string_get("service.name");
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

			if( ! ctx )
			{
				logging_printf( LOGGING_ERROR, "remote_connect_init: Unable to create socket context\n");
			} else {
				ctx->ip_address = ( char * ) strdup( found_service->ip_address );
				ctx->control_port = found_service->port;
				ctx->data_port = found_service->port+1;
				ctx->ssrc =  inv->ssrc;
				ctx->send_ssrc =  inv->ssrc;
				ctx->initiator = inv->initiator;
				ctx->seq = 0x638E;

				logging_printf( LOGGING_DEBUG, "remote_connect_init: Creating connection context for %s data=%u control=%u\n", ctx->ip_address, ctx->data_port, ctx->control_port );
				net_ctx_send( ctx, response->buffer, response->len , USE_DATA_PORT );
				ctx->seq++;
				net_ctx_send( ctx, response->buffer, response->len , USE_CONTROL_PORT );
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
	net_applemidi_inv *by = NULL;
	net_response_t *response = NULL;
	net_applemidi_command *cmd = NULL;
	char *remote_service_name = NULL;
	net_ctx_t *ctx;

	if( ! remote_connected ) return;
	remote_service_name = config_string_get("remote.connect");

	logging_printf( LOGGING_DEBUG, "remote_connect_teardown: Disconnecting from [%s]\n", remote_service_name);

	// Build the BY packet
	by = net_applemidi_inv_create();
	
	if( ! by )
	{
		logging_printf( LOGGING_ERROR, "remote_connect_teardown: Unable to allocate memory for by packet\n");
		return;
	}

	by->ssrc = remote_connect_ssrc;
	by->version = 2;
	by->initiator = remote_connect_ssrc;

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
			ctx = net_ctx_find_by_name( remote_service_name );

			if( ! ctx )
			{
				logging_printf( LOGGING_ERROR, "remote_connect_teardown: Unable to find connection for [%s]\n", remote_service_name);
			} else {
				net_ctx_send( ctx, response->buffer, response->len , USE_CONTROL_PORT);
				hex_dump( response->buffer, response->len );
			}
		}
	} else {
		logging_printf( LOGGING_ERROR, "remote_connect_teardown: Unable to create response packet\n");
	}

remote_teardown_fail:
	net_response_destroy( &response );
	net_applemidi_cmd_destroy( &cmd );
}

void remote_connect_ok( char *remote_name )
{
	net_applemidi_sync *sync_packet = NULL;
	net_response_t *response = NULL;
	net_applemidi_command *cmd = NULL;
	net_ctx_t *ctx = NULL;

	if( ! remote_name ) return;

	ctx = net_ctx_find_by_name( remote_name );

	if( ! ctx )
	{
		logging_printf( LOGGING_DEBUG, "remote_connect_ok: No context found for [%s]\n", remote_name );
		return;
	}

	remote_connected = 1;
	logging_printf(LOGGING_DEBUG, "remote_connect_ok: Connection found for [%s]\n", remote_name);

	// Build the SYNC packet
	sync_packet = net_applemidi_sync_create();
	
	if( ! sync_packet )
	{
		logging_printf( LOGGING_ERROR, "remote_connect_ok: Unable to allocate memory for sync_packet packet\n");
		return;
	}

	sync_packet->ssrc = ctx->send_ssrc;
	sync_packet->count = 0;
	sync_packet->timestamp1 = time(NULL);
	sync_packet->timestamp2 = random_number();
	sync_packet->timestamp3 = random_number();

	cmd = net_applemidi_cmd_create( NET_APPLEMIDI_CMD_SYNC );
	
	if( ! cmd ) 
	{
		logging_printf( LOGGING_ERROR, "remote_connect_ok: Unable to create AppleMIDI command\n");
		net_applemidi_sync_destroy( &sync_packet );
		goto remote_ok_fail;
	}

	cmd->data = sync_packet;

	response = net_response_create();

	if( ! response )
	{
		logging_printf( LOGGING_ERROR, "remote_connect_ok: Unable to create response packet\n");
		goto remote_ok_fail;
	}
	
		int ret = 0;
	ret = net_applemidi_pack( cmd , &(response->buffer), &(response->len) );
	if( ret != 0 )
	{
		logging_printf( LOGGING_ERROR, "remote_connect_ok: Unable to pack response to sync_packet command\n");
	} else {
		net_ctx_send( ctx, response->buffer, response->len , USE_CONTROL_PORT );
	}

remote_ok_fail:
	net_response_destroy( &response );
	net_applemidi_cmd_destroy( &cmd );
}
