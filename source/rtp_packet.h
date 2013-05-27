#ifndef RTP_PACKET_H
#define RTP_PACKET_H

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

rtp_packet_t * new_rtp_packet( void );

#endif
