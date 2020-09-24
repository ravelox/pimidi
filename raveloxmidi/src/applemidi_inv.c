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
#include <string.h>

#include <errno.h>
extern int errno;

#include "config.h"

#include "net_applemidi.h"
#include "net_connection.h"
#include "net_socket.h"
#include "net_response.h"

#include "raveloxmidi_config.h"
#include "logging.h"

#include "utils.h"

net_response_t * applemidi_inv_responder( char *ip_address, uint16_t port, void *data )
{
	net_applemidi_command *cmd = NULL;
	net_applemidi_inv *inv = NULL;
	net_applemidi_inv *accept_inv = NULL;
	net_ctx_t *ctx = NULL;
	net_response_t *response;
	char *service_name = NULL;

	if( ! data ) return NULL;

	inv = ( net_applemidi_inv *) data;

	ctx = net_ctx_find_by_ssrc( inv->ssrc );

	/* See https://en.wikipedia.org/wiki/RTP-MIDI#Apple.27s_session_protocol */

	/* If no context is found, this is a new connection */
	/* We assume that the current port is the control port */
	if( ! ctx )
	{
		logging_printf( LOGGING_INFO, "applemidi_inv_responder: Registering new connection from [%s]:[%u] [%s]\n", ip_address, port, ( inv->name ? inv->name : "") );
		ctx = net_ctx_register( inv->ssrc, inv->initiator, ip_address, port , inv->name );

		if( ! ctx ) 
		{
			logging_printf( LOGGING_ERROR, "applemidi_inv_responder: Error registering connection\n");
			return NULL;
		}
	}

	cmd = net_applemidi_cmd_create( NET_APPLEMIDI_CMD_ACCEPT );

	if( ! cmd )
	{
		logging_printf( LOGGING_ERROR, "applemidi_inv_responder: Unable to allocate memory for accept_inv command\n");
		net_ctx_reset( ctx );
		return NULL;
	}

	accept_inv = net_applemidi_inv_create();
	
	if( ! accept_inv ) {
		logging_printf( LOGGING_ERROR, "applemidi_inv_responder: Unable to allocate memory for accept_inv command data\n");
		X_FREE( cmd );
		net_ctx_reset( ctx );
		return NULL;
	}

	accept_inv->ssrc = ctx->send_ssrc;
	accept_inv->version = 2;
	accept_inv->initiator = ctx->initiator;
	service_name = config_string_get("service.name");
	if( service_name )
	{
		accept_inv->name = (char *)X_STRDUP( service_name );
	} else {
		accept_inv->name = (char *)X_STRDUP( "RaveloxMIDI" );
	}

	cmd->data = accept_inv;

	response = net_response_create();

	if( response )
	{
		int ret = 0;
		ret = net_applemidi_pack( cmd , &(response->buffer), &(response->len) );
		if( ret != 0 )
		{
			logging_printf( LOGGING_ERROR, "applemidi_inv_responder: Unable to pack response to inv command\n");
			net_response_destroy( &response );
		}
	}

	net_applemidi_cmd_destroy( &cmd );

	return response;
}
