#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include "rtp_packet.h"
#include "utils.h"

rtp_packet_t * new_rtp_packet( void )
{
	rtp_packet_t *new;

	new = ( rtp_packet_t * ) malloc( sizeof( rtp_packet_t ) );

	if( new )
	{
		memset( new, 0 , sizeof( rtp_packet_t ) );
		new->payload = NULL;
	}

	return new;
}

int rtp_packet_pack( rtp_packet_t *packet, unsigned char **out_buffer, size_t *out_buffer_len )
{
	unsigned char *p;
	size_t packed_header_buffer_size = 0;

	*out_buffer = NULL;
	*out_buffer_len = 0;

	if( ! packet )
	{
		return 1;
	}

	packed_header_buffer_size = ( sizeof(uint8_t) * 2 ) + sizeof(uint16_t) + sizeof(uint32_t);
	*out_buffer = (unsigned char *)malloc( packed_header_buffer_size );
	memset( *out_buffer, 0, packed_header_buffer_size );

	if( ! *out_buffer )
	{
		return 1;
	}
	
	*out_buffer[0] |= ( packet->header.v << 6 );
	*out_buffer[0] |= ( packet->header.p << 5 );
	*out_buffer[0] |= ( packet->header.x << 4 );
	*out_buffer[0] |= ( packet->header.cc & 0x0f );

	*out_buffer[1] |= ( packet->header.m << 7 );
	*out_buffer[1] |= ( packet->header.pt & 0x7f );

	out_buffer_len += 2;
	p = out_buffer[2];

	put_uint16( &p , packet->header.seq, out_buffer_len );
	put_uint32( &p , packet->header.timestamp, out_buffer_len );
	put_uint16( &p , packet->header.ssrc, out_buffer_len );

	return 0;
}

int rtp_gen_buffer_from_note( midi_note_packet_t *packet, unsigned char **out_buffer, size_t *out_buffer_len )
{
}

