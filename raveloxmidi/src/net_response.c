/*
   This file is part of raveloxmidi.

   Copyright (C) 2018 Dave Kelly

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
#include <unistd.h>
#include <string.h>

#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>

#include <pthread.h>

#include "net_response.h"
#include "net_applemidi.h"
#include "net_connection.h"
#include "utils.h"
#include "logging.h"
#include "config.h"

net_response_t * net_response_create( void )
{
	net_response_t *response = NULL;

	response =( net_response_t *) malloc( sizeof( net_response_t ) );

	if( response )
	{
		response->buffer = NULL;
		response->len = 0;
	}

	return response;
}

void net_response_destroy( net_response_t **response )
{
	if( ! *response ) return;

	if( (*response)->buffer )
	{
		free( (*response)->buffer );
		(*response)->buffer = NULL;
	}

	free( *response );
	*response = NULL;
}

net_response_t *net_response_inv( uint32_t ssrc, uint32_t initiator, char *name )
{
	net_applemidi_inv *inv = NULL;
	net_response_t *response = NULL;
	net_applemidi_command *cmd = NULL;

	// Build the INV packet
	inv = net_applemidi_inv_create();
	
	if( ! inv )
	{
		logging_printf( LOGGING_ERROR, "net_response_inv: Cannot create INV packet\n");
		return NULL;
	}

	inv->ssrc = ssrc;
	inv->version = 2;
	inv->initiator = initiator;
	if( name )
	{
		inv->name = (char *)strdup( name );
	} else {
		inv->name = (char *)strdup( "RaveloxMIDIClient" );
	}

	cmd = net_applemidi_cmd_create( NET_APPLEMIDI_CMD_INV );
	
	if( cmd )
	{
		cmd->data = inv;
		response = net_response_create();
		if( ! response )
		{
			logging_printf( LOGGING_ERROR, "net_response_inv: Unable to create RESPONSE packet\n");
		} else {
			int ret = 0;
			ret = net_applemidi_pack( cmd , &(response->buffer), &(response->len) );
			net_applemidi_cmd_destroy( &cmd );
			if( ret != 0 )
			{
				logging_printf( LOGGING_ERROR, "Unable to pack RESPONSE packet\n");
			} else {
				return response;
			}
		}

	} else {
		logging_printf( LOGGING_ERROR, "net_response_inv: Unable to create AppleMIDI command\n");
	}

	net_response_destroy( &response );
	net_applemidi_cmd_destroy( &cmd );

	return NULL;
}

net_response_t *net_response_sync( uint32_t send_ssrc )
{
	net_applemidi_sync *sync = NULL;
	net_response_t *response = NULL;
	net_applemidi_command *cmd = NULL;

	sync = net_applemidi_sync_create();
	
	if( ! sync )
	{
		logging_printf( LOGGING_ERROR, "net_response_sync: Unable to allocate memory for sync packet\n");
		return NULL;
	}

	sync->ssrc = send_ssrc;
	sync->count = 0;
	sync->timestamp1 = time(NULL);
	sync->timestamp2 = 0;
	sync->timestamp3 = 0;

	cmd = net_applemidi_cmd_create( NET_APPLEMIDI_CMD_SYNC );
	
	if( cmd )
	{
		cmd->data = sync;

		response = net_response_create();

		if( response )
		{
			int ret = 0;
			ret = net_applemidi_pack( cmd , &(response->buffer), &(response->len) );
			if( ret != 0 )
			{
				logging_printf( LOGGING_ERROR, "net_response_sync: Unable to pack response to sync command\n");
			} else {
				net_applemidi_cmd_destroy( &cmd );
				return response;
			}
		} else {
			logging_printf( LOGGING_ERROR, "net_response_sync: Unable to create response packet\n");
		}
	} else {
		logging_printf( LOGGING_ERROR, "net_response_sync: Unable to create AppleMIDI command\n");
	}

	net_response_destroy( &response );
	net_applemidi_cmd_destroy( &cmd );

	return NULL;
}
