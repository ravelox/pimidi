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

#include "net_applemidi.h"
#include "net_connection.h"
#include "net_listener.h"
#include "midi_journal.h"

#include "logging.h"

net_response_t * cmd_feedback_handler( void *data )
{
	net_applemidi_feedback  *feedback;
	net_ctx_t *ctx = NULL;

	if( ! data ) return NULL;

	feedback = ( net_applemidi_feedback *) data;

	logging_printf( LOGGING_DEBUG, "FEEDBACK( \n");
	logging_printf( LOGGING_DEBUG, "\tssrc=0x%08x\n", feedback->ssrc);
	logging_printf( LOGGING_DEBUG, "\tappleseq=%u\n", feedback->apple_seq);
	logging_printf( LOGGING_DEBUG, "\trtpseq=%u )\n", feedback->rtp_seq[1]);

	ctx = net_ctx_find_by_ssrc( feedback->ssrc );

	if( ! ctx ) return NULL;

	logging_printf( LOGGING_DEBUG, "Context found ( %u, %u )\n", feedback->rtp_seq[1], ctx->seq );
	if( feedback->rtp_seq[1] >= ctx->seq )
	{
		logging_printf( LOGGING_DEBUG, "cmd_feedback_handler: Resetting journal\n" );
		journal_reset( ctx->journal );
	}

	return NULL;
}
