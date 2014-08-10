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
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include "rtp_packet.h"
#include "utils.h"

#include "logging.h"

rtp_packet_t * rtp_packet_create( void )
{
	rtp_packet_t *new;

	new = ( rtp_packet_t * ) malloc( sizeof( rtp_packet_t ) );

	if( new )
	{
		memset( new, 0 , sizeof( rtp_packet_t ) );
		new->payload = NULL;

		// Initialise the standard fields

		new->header.v = RTP_VERSION;
		new->header.p = 0;
		new->header.x = 0;
		new->header.cc = 0;
		new->header.m = 0;
		new->header.pt = RTP_DYNAMIC_PAYLOAD_97;
	}

	return new;
}

int rtp_packet_destroy( rtp_packet_t **packet )
{

	if( ! packet ) return 1;
	if( ! *packet ) return 1;

	(*packet)->payload = NULL;
	(*packet)->payload_len = 0;
	FREENULL( (void **)packet);
	return 0;
}


int rtp_packet_pack( rtp_packet_t *packet, unsigned char **out_buffer, size_t *out_buffer_len )
{
	unsigned char *p;
	size_t packed_header_buffer_size = 0;
	uint16_t temp_header = 0;

	*out_buffer = NULL;
	*out_buffer_len = 0;

	if( ! packet )
	{
		return 1;
	}

	packed_header_buffer_size = ( sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint16_t) );
	*out_buffer = (unsigned char *)malloc( packed_header_buffer_size );
	memset( *out_buffer, 0, packed_header_buffer_size );

	if( ! *out_buffer )
	{
		return 1;
	}
	
	p = *out_buffer;

	temp_header |= ( packet->header.v << 6 ) << 8;
	temp_header |= ( packet->header.p << 5 ) << 8;
	temp_header |= ( packet->header.x << 4 ) << 8;
	temp_header |= ( packet->header.cc & 0x0f ) << 8;
	temp_header |= ( packet->header.m << 7 );
	temp_header |= ( packet->header.pt & 0x7f );

	put_uint16( &p , temp_header,  out_buffer_len );
	put_uint16( &p , packet->header.seq, out_buffer_len );
	put_uint32( &p , packet->header.timestamp, out_buffer_len );
	put_uint32( &p , packet->header.ssrc, out_buffer_len );

	if( packet->payload && (packet->payload_len > 0) )
	{
		*out_buffer = (unsigned char *)realloc( *out_buffer, *out_buffer_len + packet->payload_len );
		if( *out_buffer )
		{
			p = *out_buffer + *out_buffer_len;
			memcpy( p , packet->payload, packet->payload_len );
			*out_buffer_len += packet->payload_len;
		}
	}

	return 0;
}

void rtp_packet_dump( rtp_packet_t *packet )
{
	if( ! packet ) return;

	logging_printf( LOGGING_DEBUG, "RTP Packet( ");
	logging_printf( LOGGING_DEBUG, "V=%u , ", packet->header.v);
	logging_printf( LOGGING_DEBUG, "P=%u , ", packet->header.p);
	logging_printf( LOGGING_DEBUG, "X=%u , ", packet->header.x);
	logging_printf( LOGGING_DEBUG, "CC=%u , ", packet->header.cc);
	logging_printf( LOGGING_DEBUG, "M=%u , ", packet->header.m);
	logging_printf( LOGGING_DEBUG, "PT=%u , ", packet->header.pt);
	logging_printf( LOGGING_DEBUG, "seq=%u , ", packet->header.seq);
	logging_printf( LOGGING_DEBUG, "timestamp= 0x%08x , ", packet->header.timestamp );
	logging_printf( LOGGING_DEBUG, "ssrc=0x%08x , ", packet->header.ssrc );
	logging_printf( LOGGING_DEBUG, "payloadlength=%u , ", packet->payload_len);
	logging_printf( LOGGING_DEBUG, "payload=%p )\n", packet->payload);

	return;
}
