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

#include "logging.h"

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
