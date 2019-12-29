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
#include <time.h>

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

#include "midi_journal.h"

#include "logging.h"

net_response_t * applemidi_feedback_responder( void *data )
{
	net_applemidi_feedback  *feedback;
	net_ctx_t *ctx = NULL;

	if( ! data ) return NULL;

	feedback = ( net_applemidi_feedback *) data;

	ctx = net_ctx_find_by_ssrc( feedback->ssrc );

	if( ! ctx )
	{
		logging_printf(LOGGING_DEBUG,"applemidi_feedback_responder: No context found (search=%u)\n", feedback->rtp_seq[1]);
		return NULL;
	}

	logging_printf( LOGGING_DEBUG, "applemidi_feedback_responder: Context found ( search=%u, found=%u )\n", feedback->rtp_seq[1], ctx->seq );
	if( feedback->rtp_seq[1] >= ctx->seq )
	{
		logging_printf( LOGGING_DEBUG, "applemidi_feedback_responder: Resetting journal\n" );
		net_ctx_journal_reset(ctx);
	}

	return NULL;
}

net_response_t *applemidi_feedback_create( uint32_t ssrc, uint16_t rtp_seq )
{
	net_applemidi_command *cmd = NULL;
	net_applemidi_feedback *feedback = NULL;
	net_response_t *response = NULL;

	feedback = net_applemidi_feedback_create();

	if( ! feedback ) return NULL;

	cmd = net_applemidi_cmd_create( NET_APPLEMIDI_CMD_FEEDBACK );

	if( ! cmd )
	{
		free( feedback );
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
