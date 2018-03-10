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
	size_t payload_len;
	void *payload;
} rtp_packet_t;

#define RTP_VERSION 2

#define RTP_DYNAMIC_PAYLOAD_97	97

rtp_packet_t * rtp_packet_create( void );
void rtp_packet_destroy( rtp_packet_t **packet );
int rtp_packet_pack( rtp_packet_t *packet, unsigned char **out_buffer, size_t *out_buffer_len );
void rtp_packet_unpack( unsigned char *buffer, size_t buffer_len, rtp_packet_t *rtp_packet );
void rtp_packet_dump( rtp_packet_t *packet );

#endif
