#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include "midi_payload.h"
#include "utils.h"

void midi_payload_destroy( midi_payload_t **payload )
{
	if( ! payload ) return;
	if( ! *payload ) return;

	if( (*payload)->buffer )
	{
		FREENULL( (void **)&((*payload)->buffer) );
	}

	FREENULL( (void **)payload );
}

void payload_reset( midi_payload_t **payload )
{
	if( ! payload ) return;
	if( ! *payload ) return;

	memset( *payload, 0, sizeof( midi_payload_t ) );
	(*payload)->buffer = NULL;
}

midi_payload_t * midi_payload_create( void )
{
	midi_payload_t *payload = NULL;

	payload = ( midi_payload_t * )malloc( sizeof( midi_payload_t ) );

	if( ! payload ) return NULL;

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

void payload_set_buffer( midi_payload_t *payload, char *buffer , uint16_t buffer_size)
{
	if( ! payload ) return;

	payload->header->len = buffer_size;
	payload->buffer = buffer;
}


void payload_pack( midi_payload_t *payload, unsigned char **buffer, size_t *buffer_size)
{
	uint16_t temp_header;

	*buffer = NULL;
	*buffer_size = 0;

	if( ! payload ) return;

	if( ! payload->buffer ) return;
	if( ! payload->header ) return;

	*buffer_size = payload->header->len + (payload->header->len > 15 ? 1 : 0);
	*buffer = (unsigned char *)malloc( *buffer_size );

	if( ! *buffer ) return;

	if( payload->header->B) temp_header |= PAYLOAD_HEADER_B << 8;
	if( payload->header->J) temp_header |= PAYLOAD_HEADER_J << 7;
	if( payload->header->Z) temp_header |= PAYLOAD_HEADER_Z << 6;
	if( payload->header->P) temp_header |= PAYLOAD_HEADER_P << 5;

	if( payload->header->len > 15 )
	{
		temp_header |= (payload->header->len & 0x0f00 ) << 4;
		temp_header |= (payload->header->len & 0x00ff);

		(*buffer)[1] = ( temp_header & 0x00ff );
	} else  {
		temp_header |= (payload->header->len & 0x0f );
	}

	(*buffer)[0] = ( temp_header & 0xff00 ) >> 8;

	memcpy( *buffer, payload->buffer, payload->header->len );
}
