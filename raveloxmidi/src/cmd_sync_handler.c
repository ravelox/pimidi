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

net_response_t * cmd_sync_handler( void *data )
{
	net_applemidi_command *cmd = NULL;
	net_applemidi_sync *sync = NULL;
	net_applemidi_sync *sync_resp = NULL;
	net_ctx_t *ctx = NULL;
	net_response_t *response = NULL;
	uint32_t delta = 0;
	int ret = 0;

	if( ! data ) return NULL;

	sync = ( net_applemidi_sync *) data;

	fprintf(stderr, "SYNC( ");

	fprintf(stderr, "ssrc=0x%08x , ", sync->ssrc);
	fprintf(stderr, "count=0x%08x , ", sync->count);
	fprintf(stderr, "timestamp1=0x%llu , ", sync->timestamp1);
	fprintf(stderr, "timestamp2=0x%llu , ", sync->timestamp2);
	fprintf(stderr, "timestamp3=0x%llu )\n", sync->timestamp3);

	ctx = net_ctx_find_by_ssrc( sync->ssrc);

	if( ! ctx ) return NULL;

	cmd = new_net_applemidi_command( NET_APPLEMIDI_CMD_SYNC );

	if( ! cmd )
	{
		fprintf(stderr, "Unable to allocate memory for sync command\n");
		return NULL;
	}

	sync_resp = new_net_applemidi_sync();
	
	if( ! sync_resp ) {
		fprintf(stderr, "Unabled to allocate memory for sync_resp command data\n");
		free( cmd );
		return NULL;
	}

	sync_resp->ssrc = ctx->send_ssrc;
	sync_resp->count = ( sync->count < 2 ? sync->count + 1 : 0 );
	
	switch( sync_resp->count )
	{
		case 2:
			delta = time( NULL ) - ctx->start;
			sync_resp->timestamp3 = delta;
			sync_resp->timestamp2 = delta;
			sync_resp->timestamp1 = delta;
			break;
		case 1:
			delta = time( NULL ) - ctx->start;
			sync_resp->timestamp2 = delta;
			sync_resp->timestamp1 = delta;
			break;
		case 0:
			delta = time( NULL ) - ctx->start;
			sync_resp->timestamp1 = delta;
			break;
	}

	cmd->data = sync_resp;

	response = new_net_response();

	if( response )
	{
		ret = net_applemidi_pack( cmd , &(response->buffer), &(response->len) );
	}

	net_applemidi_cmd_destroy( &cmd );

	return response;
}
