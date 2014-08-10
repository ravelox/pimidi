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

#include "net_applemidi.h"
#include "net_connection.h"
#include "net_listener.h"

#include "logging.h"

net_response_t * cmd_end_handler( void *data )
{
	net_applemidi_inv *inv = NULL;
	net_ctx_t *ctx = NULL;

	if( ! data ) return NULL;

	inv = ( net_applemidi_inv *) data;

	logging_printf( LOGGING_DEBUG, "BYE( ");

	logging_printf( LOGGING_DEBUG, "name=<<%s>> , ", inv->name);
	logging_printf( LOGGING_DEBUG, "ssrc=0x%08x , ", inv->ssrc);
	logging_printf( LOGGING_DEBUG, "version = 0x%08x , ", inv->version);
	logging_printf( LOGGING_DEBUG, "initiator=0x%08x )\n", inv->initiator);

	ctx = net_ctx_find_by_ssrc( inv->ssrc );

	if( ! ctx )
	{
		logging_printf( LOGGING_DEBUG, "No existing connection found\n");
		return NULL;
	}

	net_ctx_reset( ctx );

	return NULL;
}
