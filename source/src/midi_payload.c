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

midi_payload_t * midi_payload_create( void )
{
	midi_payload_t *payload = NULL;

	payload = ( midi_payload_t * )malloc( sizeof( midi_payload_t ) );

	if( ! payload ) return NULL;

	memset( payload, 0, sizeof( midi_payload_t ) );
	payload->buffer = NULL;

	return payload;
}

void payload_set_b( midi_payload_t *payload )
{
	if(! payload) return;

	payload->header |= PAYLOAD_HEADER_B;
}

void payload_set_j( midi_payload_t *payload )
{
	if( ! payload ) return;

	payload->header |= PAYLOAD_HEADER_J;
}

void payload_set_z( midi_payload_t *payload )
{
	if( ! payload ) return;

	payload->header |= PAYLOAD_HEADER_Z;
}

void payload_set_p( midi_payload_t *payload )
{
	if( ! payload ) return;

	payload->header |= PAYLOAD_HEADER_P;
}

void payload_set_buffer( midi_payload_t *payload, char *buffer )
{
	if( ! payload ) return;

	payload->buffer = buffer;
}
