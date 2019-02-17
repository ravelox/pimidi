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
#include "logging.h"

net_response_t * applemidi_ok_responder( char *ip_address, uint16_t port, void *data )
{
	net_applemidi_inv *inv = NULL;

	if( ! data ) return NULL;
	inv = ( net_applemidi_inv *) data;

/*
	accept_inv->ssrc = ctx->send_ssrc;
	accept_inv->version = 2;
	accept_inv->initiator = ctx->initiator;
	service_name = config_string_get("service.name");
*/

	logging_printf( LOGGING_DEBUG, "applemidi_ok_responder: ssrc=0x%08x version=%u initiator=0x%08x name=%s\n", inv->ssrc, inv->version, inv->initiator, inv->name);
	return NULL;
}
