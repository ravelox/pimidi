#ifndef RTP_PACKET_H
#define RTP_PACKET_H

#include "midi_note_packet.h"

typedef struct rtp_packet_header_t {
	unsigned	v:2;
	unsigned	p:1;
	unsigned	x:1;
	unsigned	cc:4;
	unsigned	m:1;
	unsigned	pt:7;
	uint16_t	seq;
	uint32_t	timestamp;
	uint32_t	ssrc;
} rtp_packet_header_t;

typedef struct rtp_packet_t {
	rtp_packet_header_t header;
	void *payload;
} rtp_packet_t;

#define RTP_VERSION 2

#define RTP_DYNAMIC_PAYLOAD_97	97

rtp_packet_t * rtp_packet_create( void );
int rtp_packet_destroy( rtp_packet_t **packet );

#endif
