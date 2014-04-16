#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include <arpa/inet.h>

#include "midi_journal.h"
#include "net_connection.h"
#include "rtp_packet.h"
#include "utils.h"

static net_ctx_t *ctx[ MAX_CTX ];

void net_ctx_reset( net_ctx_t *ctx )
{
	journal_t *journal;

	if( ! ctx ) return;

	ctx->used = 0;
	ctx->ssrc = 0;
	ctx->send_ssrc = 0;
	ctx->initiator = 0;
	ctx->seq = 0x638F;
	FREENULL( (void **)&(ctx->ip_address) );
	journal_destroy( &(ctx->journal) );
	
	journal_init( &journal );
	ctx->journal = journal;
}

void debug_net_ctx_dump( net_ctx_t *ctx )
{
	if( ! ctx ) return;
	
	fprintf(stderr, "CTX( ");
	fprintf(stderr, "Used=%d , ", ctx->used);
	fprintf(stderr, "ssrc=%08x , ", ctx->ssrc);
	fprintf(stderr, "send_ssrc=%08x , ", ctx->send_ssrc);
	fprintf(stderr, "initiator=%08x , ", ctx->initiator);
	fprintf(stderr, "seq=%08x (%08d) , ", ctx->seq, ctx->seq);
	fprintf(stderr, "host=%s:%u )\n", ctx->ip_address, ctx->port);
}

static void net_ctx_set( net_ctx_t *ctx, uint32_t ssrc, uint32_t initiator, uint32_t send_ssrc, uint32_t seq, uint16_t port, char *ip_address )
{
	if( ! ctx ) return;

	if( ctx->used > 0 ) return;

	net_ctx_reset( ctx );

	ctx->used = 1;
	ctx->ssrc = ssrc;
	ctx->send_ssrc = send_ssrc;
	ctx->initiator = initiator;
	ctx->seq = 0x638F;
	ctx->port = port;
	ctx->start = time( NULL );

	ctx->ip_address = ( char *) strdup( ip_address );

}

static net_ctx_t * new_net_ctx( void )
{
	net_ctx_t *new_ctx;
	journal_t *journal;

	new_ctx = ( net_ctx_t * ) malloc( sizeof( net_ctx_t ) );

	memset( new_ctx, 0, sizeof( net_ctx_t ) );
	new_ctx->seq = 0x638F;

	journal_init( &journal );

	new_ctx->journal = journal;

	return new_ctx;

}

void net_ctx_init( void )
{
	uint8_t i;

	for( i = 0 ; i < MAX_CTX ; i++ )
	{
		ctx[ i ] = new_net_ctx();
	}
}


void net_ctx_destroy( void )
{
	uint8_t i;

	for( i = 0 ; i < MAX_CTX ; i++ )
	{
		if( ctx[ i ] )
		{
			net_ctx_reset( ctx[i] );
			journal_destroy( & (ctx[i]->journal) );
			free( ctx[i] );
			ctx[i] = NULL;
		}
	}
}

net_ctx_t * net_ctx_find_by_id( uint8_t id )
{
	if( id < MAX_CTX )
	{
		if( ctx[id] )
		{
			if( ctx[id]->used > 0 )
			{
				return ctx[id];
			}
		}
	}
	return NULL;
}

net_ctx_t * net_ctx_find_by_ssrc( uint32_t ssrc)
{
	uint8_t i;

	for( i = 0 ; i < MAX_CTX ; i++ )
	{
		if( ctx[i] )
		{
			if( ctx[i]->used > 0 )
			{
				if( ctx[i]->ssrc == ssrc )
				{
					return ctx[i];
				}
			}
		}
	}

	return NULL;
}

net_ctx_t * net_ctx_register( uint32_t ssrc, uint32_t initiator, char *ip_address, uint16_t port )
{
	uint8_t i;

	for( i = 0 ; i < MAX_CTX ; i++ )
	{
		if( ctx[i] )
		{
			if( ctx[i]->used == 0 )
			{
				time_t now = time( NULL );
				unsigned int send_ssrc = rand_r( (unsigned int *)&now );

				net_ctx_set( ctx[i], ssrc, initiator, send_ssrc, 0x638F, port, ip_address );
				return ctx[i];
			}
		}
	}
	
	return NULL;
}

void net_ctx_add_journal_note( uint8_t ctx_id , char channel, char note, char velocity )
{
	if( ctx_id > MAX_CTX - 1 ) return;

	ctx[ctx_id]->seq += 1;

	midi_journal_add_note( ctx[ctx_id]->journal, ctx[ctx_id]->seq, channel, note, velocity );
}

void debug_ctx_journal_dump( uint8_t ctx_id )
{
	char *buffer = NULL;
	size_t size = 0;

	if( ctx_id > MAX_CTX - 1 ) return;

	fprintf(stderr, "Journal has data (NULLTEST) -----: %u\n", journal_has_data( NULL ));
	fprintf(stderr, "Journal has data ----------------: %u\n", journal_has_data( ctx[ctx_id]->journal ));

	if( ! journal_has_data( ctx[ctx_id]->journal ) ) return;

	journal_dump( ctx[ctx_id]->journal );

	journal_pack( ctx[ctx_id]->journal, &buffer, &size );
	FREENULL( (void **)&buffer );

	net_ctx_reset( ctx[ctx_id] );
}

void net_ctx_journal_pack( uint8_t ctx_id, char **journal_buffer, size_t *journal_buffer_size)
{
	net_ctx_t *ctx = NULL;

	*journal_buffer = NULL;
	*journal_buffer_size = 0;

	ctx = net_ctx_find_by_id( ctx_id );

	if( ! ctx) return;

	journal_pack( ctx->journal, journal_buffer, journal_buffer_size );
}
	
void net_ctx_journal_reset( uint8_t ctx_id )
{
	if( ctx_id > MAX_CTX - 1) return;

	journal_reset( ctx[ctx_id]->journal);
}

void net_ctx_update_rtp_fields( uint8_t ctx_id, rtp_packet_t *rtp_packet)
{
	net_ctx_t *ctx = NULL;

	if( ! rtp_packet ) return;

	ctx = net_ctx_find_by_id( ctx_id );

	if( ! ctx ) return;

	rtp_packet->header.seq = ctx->seq;
	rtp_packet->header.timestamp = time(0) - ctx->start;
	rtp_packet->header.ssrc = ctx->send_ssrc;
}

void net_ctx_send( uint8_t ctx_id, int send_socket, unsigned char *buffer, size_t buffer_len )
{
	net_ctx_t *ctx = NULL;
	struct sockaddr_in send_address;
	size_t bytes_sent = 0;

	if(  send_socket < 0 ) return;
	if( ! buffer ) return;
	if( buffer_len <= 0 ) return;

	ctx = net_ctx_find_by_id( ctx_id );

	if( ! ctx ) return;

	debug_net_ctx_dump( ctx );

	memset((char *)&send_address, 0, sizeof( send_address));
	send_address.sin_family = AF_INET;
	send_address.sin_port = htons( ctx->port + 1) ;
	inet_aton( ctx->ip_address, &send_address.sin_addr );

	bytes_sent = sendto( send_socket, buffer, buffer_len , 0 , (struct sockaddr *)&send_address, sizeof( send_address ) );
	fprintf(stderr, "Sent %u bytes to connection\n", bytes_sent);
}
