/*

MIDI Payload ( with journal ) is like this:

43 80 27 00 20 63 90 00 07 08 81 F1 27 7F

// Channel header
Set BJZP flags = 0000 ( if no journal ) and 0100 ( if journal )
Set length = 3

// MIDI Note ON/OFF
0x90 NOTE ON 0x80 NOTE OFF
NOTE
VELOCITY

// Journal header
Set SYAH = 0010
Set totchan = 0000
SEQ NUM

// Channel header
Set S=0
Set channel = 0000
Set H=0
Set Length = 0000000111
Set chapter flags
   N=1

// Chapter N header
Set B=1
Set len=1
Set low=1111
Set high=0001

*/

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
