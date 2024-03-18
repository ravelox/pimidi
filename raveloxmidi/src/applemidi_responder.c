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

#include "utils.h"

#include "net_applemidi.h"
#include "net_connection.h"
#include "net_socket.h"
#include "net_response.h"
#include "midi_journal.h"

#include "logging.h"

#include "raveloxmidi_config.h"
#include "remote_connection.h"

void applemidi_by_responder( void *data )
{
	net_applemidi_inv *inv = NULL;
	net_ctx_t *ctx = NULL;

	logging_printf( LOGGING_DEBUG, "applemidi_by_responder: data=%p\n", data );
	if( ! data ) return;

	inv = ( net_applemidi_inv *) data;

	ctx = net_ctx_find_by_ssrc( inv->ssrc );

	if( ! ctx )
	{
		logging_printf( LOGGING_WARN, "applemidi_by_responder:No existing connection found\n");
		return;
	}

	net_ctx_reset( ctx );
	net_ctx_dump( ctx );

	return;
}

void applemidi_feedback_responder( void *data )
{
	net_applemidi_feedback  *feedback;
	net_ctx_t *ctx;
	uint32_t seq = 0;

	if( ! data ) return;

	feedback = ( net_applemidi_feedback *) data;

	ctx = net_ctx_find_by_ssrc( feedback->ssrc );

	if( ! ctx )
	{
		logging_printf(LOGGING_DEBUG,"applemidi_feedback_responder: No context found (search=%u)\n", feedback->rtp_seq[1]);
		return;
	}

	net_ctx_lock( ctx );
	seq = ctx->seq;
	net_ctx_unlock( ctx );

	logging_printf( LOGGING_DEBUG, "applemidi_feedback_responder: Context found ( search=%u, found=%u )\n", feedback->rtp_seq[1], seq );
	if( feedback->rtp_seq[1] >= seq )
	{
		logging_printf( LOGGING_DEBUG, "applemidi_feedback_responder: Resetting journal\n" );
		net_ctx_journal_reset(ctx);
	}
}

net_response_t *applemidi_feedback_create( uint32_t ssrc, uint16_t rtp_seq )
{
	net_applemidi_command *cmd = NULL;
	net_applemidi_feedback *feedback = NULL;
	net_response_t *response = NULL;

	feedback = net_applemidi_feedback_create();

	if( feedback == NULL  ) return NULL;

	cmd = net_applemidi_cmd_create( NET_APPLEMIDI_CMD_FEEDBACK );

	if( ! cmd )
	{
		X_FREE( feedback );
		return NULL;
	}
	
	feedback->ssrc = ssrc;
	feedback->rtp_seq[1] = rtp_seq;

	cmd->data = feedback;

	response = net_response_create();
	if( response )
	{
		net_applemidi_pack( cmd, &(response->buffer), &(response->len) );
	}

	net_applemidi_cmd_destroy( &cmd );

	return response;
}

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

net_response_t * applemidi_sync_responder( void *data )
{
	net_applemidi_command *cmd = NULL;
	net_applemidi_sync *sync = NULL;
	net_applemidi_sync *sync_resp = NULL;
	net_ctx_t *ctx = NULL;
	net_response_t *response = NULL;
	unsigned long local_timestamp = 0;
	unsigned long current_time = 0;

	if( ! data ) return NULL;

	sync = ( net_applemidi_sync *) data;

	ctx = net_ctx_find_by_ssrc( sync->ssrc);

	if( ! ctx )
	{
		logging_printf( LOGGING_DEBUG, "applemidi_sync_responder: No context found for ssrc=0x%08x\n", sync->ssrc );
		return NULL;
	}

	net_ctx_dump( ctx );

        if ( sync->count == 2 )
	{
                long long offset_estimate = ((sync->timestamp3 + sync->timestamp1) / 2) - sync->timestamp2;
                current_time = time_in_microseconds();
                local_timestamp = current_time - ctx->start;
                logging_printf( LOGGING_DEBUG, "applemidi_sync_responder: CK2 Received. Sync Done. now=%lu start=%lu local_timestamp=%lu offset_estimate=%lu\n", current_time, ctx->start, local_timestamp, offset_estimate );
                return NULL;
        }

	cmd = net_applemidi_cmd_create( NET_APPLEMIDI_CMD_SYNC );

	if( ! cmd )
	{
		logging_printf( LOGGING_ERROR, "applemidi_sync_responder: Unable to allocate memory for sync command\n");
		return NULL;
	}

	sync_resp = net_applemidi_sync_create();
	
	if( ! sync_resp ) {
		logging_printf( LOGGING_ERROR, "applemidi_sync_responder: Unable to allocate memory for sync_resp command data\n");
		X_FREE( cmd );
		return NULL;
	}

	sync_resp->ssrc = ctx->send_ssrc;
	sync_resp->count = ( sync->count < 2 ? sync->count + 1 : 0 );

	/* Copy the timestamps from the SYNC command */
	sync_resp->timestamp1 = sync->timestamp1;
	sync_resp->timestamp2 = sync->timestamp2;
	sync_resp->timestamp3 = sync->timestamp3;

	memcpy( sync_resp->padding, sync->padding, 3 );

	current_time = time_in_microseconds();
	local_timestamp = current_time - ctx->start;

	logging_printf( LOGGING_DEBUG, "applemidi_sync_responder: now=%lu start=%lu local_timestamp=%lu\n", current_time, ctx->start, local_timestamp );
	
	switch( sync_resp->count )
	{
		case 2:
			sync_resp->timestamp3 = local_timestamp;
			break;
		case 1:
			sync_resp->timestamp2 = local_timestamp;
			break;
		case 0:
			sync_resp->timestamp1 = local_timestamp;
			break;
	}

	cmd->data = sync_resp;

	net_applemidi_command_dump( cmd );

	response = net_response_create();

	if( response )
	{
		int ret = 0;
		ret = net_applemidi_pack( cmd , &(response->buffer), &(response->len) );
		if( ret != 0 )
		{
			logging_printf( LOGGING_ERROR, "applemidi_sync_responder: Unable to pack response to sync command\n");
			net_response_destroy( &response );
			response = NULL;
		}
	}

	net_applemidi_cmd_destroy( &cmd );

	return response;
}
