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
#include <unistd.h>

#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>

#include <errno.h>
extern int errno;

#include "config.h"

#include "net_applemidi.h"
#include "net_connection.h"
#include "net_socket.h"
#include "net_response.h"

#include "raveloxmidi_config.h"
#include "utils.h"
#include "logging.h"

#include "remote_connection.h"

net_response_t * applemidi_ok_responder( char *ip_address, uint16_t port, void *data )
{
	net_applemidi_inv *ok_packet = NULL;
	net_ctx_t *ctx = NULL;
	net_response_t *response = NULL;

	if( ! data ) return NULL;

	ok_packet = ( net_applemidi_inv *) data;
	logging_printf( LOGGING_DEBUG, "applemidi_ok_responder: address=[%s]:%u ssrc=0x%08x version=%u initiator=0x%08x name=%s\n", ip_address, port, ok_packet->ssrc, ok_packet->version, ok_packet->initiator, ok_packet->name);

	ctx = net_ctx_find_by_initiator( ok_packet->initiator );

	if( ! ctx )
	{
		logging_printf(LOGGING_WARN,"applemidi_ok_responder: Unexpected OK from ssrc=0x%08x\n", ok_packet->ssrc);
		return NULL;
	}

	logging_printf( LOGGING_DEBUG, "applemidi_ok_responder: address=[%s]:%u status=%s\n", ip_address, port, net_ctx_status_to_string(ctx->status ));
	switch( ctx->status )
	{
		case NET_CTX_STATUS_FIRST_INV:
			response = net_response_inv( ctx->send_ssrc, ctx->initiator, config_string_get("service.name") );
			ctx->ssrc = ok_packet->ssrc;
			net_ctx_send( ctx, response->buffer, response->len , USE_DATA_PORT );
			hex_dump( response->buffer, response->len );
			net_response_destroy( &response );
			response = NULL;
			ctx->status = NET_CTX_STATUS_SECOND_INV;
			break;
		case NET_CTX_STATUS_SECOND_INV:
			response = net_response_sync( ctx->send_ssrc , ctx->start );
			net_ctx_send( ctx, response->buffer, response->len, USE_CONTROL_PORT );
			hex_dump( response->buffer, response->len );
			net_response_destroy( &response );
			response = NULL;
			ctx->status = NET_CTX_STATUS_REMOTE_CONNECTION;
			logging_printf( LOGGING_INFO, "Remote connection established to [%s]\n", ok_packet->name );
			remote_connect_sync_start();
			break;
		default:
			break;
	}

	return response;
}
