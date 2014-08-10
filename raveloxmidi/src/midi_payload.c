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

#include "midi_payload.h"
#include "utils.h"

#include "logging.h"

void midi_payload_destroy( midi_payload_t **payload )
{
	if( ! payload ) return;
	if( ! *payload ) return;

	(*payload)->buffer = NULL;

	if( (*payload)->header )
	{
		FREENULL( (void **)&((*payload)->header) );
	}

	FREENULL( (void **)payload );
}

void payload_reset( midi_payload_t **payload )
{
	if( ! payload ) return;
	if( ! *payload ) return;

	(*payload)->buffer = NULL;

	if( (*payload)->header )
	{
		(*payload)->header->B = 0;
		(*payload)->header->J = 0;
		(*payload)->header->Z = 0;
		(*payload)->header->P = 0;
		(*payload)->header->len = 0;
	}
}

midi_payload_t * midi_payload_create( void )
{
	midi_payload_t *payload = NULL;

	payload = ( midi_payload_t * )malloc( sizeof( midi_payload_t ) );

	if( ! payload ) return NULL;

	payload->header = ( midi_payload_header_t *)malloc( sizeof( midi_payload_header_t ) );
	if( ! payload->header )
	{
		free( payload );
		return NULL;
	}
	payload->buffer = NULL;

	payload_reset( &payload );

	return payload;
}

void payload_toggle_b( midi_payload_t *payload )
{
	if(! payload) return;

	payload->header->B ^= 1;
}

void payload_toggle_j( midi_payload_t *payload )
{
	if( ! payload ) return;

	payload->header->J ^= 1;
}

void payload_toggle_z( midi_payload_t *payload )
{
	if( ! payload ) return;

	payload->header->Z ^=1;
}

void payload_toggle_p( midi_payload_t *payload )
{
	if( ! payload ) return;

	payload->header->P ^= 1;
}

void payload_set_buffer( midi_payload_t *payload, unsigned char *buffer , uint16_t buffer_size)
{
	if( ! payload ) return;

	payload->header->len = buffer_size;
	payload->buffer = buffer;
}


void payload_dump( midi_payload_t *payload )
{
	if( ! payload ) return;
	if( ! payload->header ) return;

	logging_printf( LOGGING_DEBUG, "MIDI Payload( ");
	logging_printf( LOGGING_DEBUG, "B=%d , ", payload->header->B);
	logging_printf( LOGGING_DEBUG, "J=%d , ", payload->header->J);
	logging_printf( LOGGING_DEBUG, "Z=%d , ", payload->header->Z);
	logging_printf( LOGGING_DEBUG, "P=%d , ", payload->header->P);
	logging_printf( LOGGING_DEBUG, "payloadlength=%u )\n", payload->header->len);
}

void payload_pack( midi_payload_t *payload, unsigned char **buffer, size_t *buffer_size)
{
	uint8_t temp_header = 0;
	unsigned char *p = NULL;

	*buffer = NULL;
	*buffer_size = 0;

	if( ! payload ) return;

	if( ! payload->buffer ) return;
	if( ! payload->header ) return;

	payload_dump( payload );

	*buffer_size = 1 + payload->header->len + (payload->header->len > 15 ? 1 : 0);
	*buffer = (unsigned char *)malloc( *buffer_size );

	if( ! *buffer ) return;

	p = *buffer;

	if( payload->header->B) temp_header |= PAYLOAD_HEADER_B;
	if( payload->header->J) temp_header |= PAYLOAD_HEADER_J;
	if( payload->header->Z) temp_header |= PAYLOAD_HEADER_Z;
	if( payload->header->P) temp_header |= PAYLOAD_HEADER_P;

	*p = temp_header;

	if( payload->header->len <= 15 )
	{
		*p |= ( payload->header->len & 0x0f );
		p++;
	} else {
		temp_header |= (payload->header->len & 0x0f00 ) >> 8;
		*p = temp_header;
		p++;
		*p =  (payload->header->len & 0x00ff);
		p++;
	}

	memcpy( p, payload->buffer, payload->header->len );
}
