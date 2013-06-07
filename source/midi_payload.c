#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include "midi_payload.h"
#include "utils.h"

midi_payload_header_t * midi_payload_header_create( void )
{
	midi_payload_header_t *header = NULL;

	header = (midi_payload_header_t *)malloc( sizeof( midi_payload_header_t ) );

	if( ! header) return NULL;

	memset( header, 0 , sizeof( midi_payload_header_t ) );

	return header;
}

void midi_payload_header_destroy( midi_payload_header_t **header )
{
	FREENULL( (void **)header );
}

void midi_payload_destroy( midi_payload_t **payload )
{
	if( ! payload ) return;
	if( ! *payload ) return;

	if( (*payload)->header )
	{
		midi_payload_header_destroy( &( (*payload)->header) );
	}

	FREENULL( (void **)payload );
}

midi_payload_t * midi_payload_create( void )
{
	midi_payload_t *payload = NULL;

	payload = ( midi_payload_t * )malloc( sizeof( midi_payload_t ) );

	if( ! payload ) return NULL;

	payload->header = midi_payload_header_create();

	if( ! payload->header )
	{
		midi_payload_destroy( &payload );
		return NULL;
	}

	return payload;
}
