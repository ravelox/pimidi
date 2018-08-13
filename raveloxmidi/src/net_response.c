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

#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>

#include <pthread.h>

#include "net_response.h"
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
