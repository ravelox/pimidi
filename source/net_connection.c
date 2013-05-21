#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include <arpa/inet.h>

#include "net_connection.h"

static net_ctx_t *ctx[ MAX_CTX ];

void net_ctx_reset( net_ctx_t *ctx )
{
	if( ! ctx ) return;

	ctx->used = 0;
	ctx->ssrc = 0;
	ctx->send_ssrc = 0;
	ctx->initiator = 0;
	ctx->seq = 0;
	if( ctx->ip_address )
	{
		free( ctx->ip_address );
	}
	ctx->ip_address = NULL;
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
	ctx->seq = 1;
	ctx->port = port;
	ctx->start = time( NULL );

	ctx->ip_address = ( char *) strdup( ip_address );

}

static net_ctx_t * new_net_ctx( void )
{
	net_ctx_t *new_ctx;

	new_ctx = ( net_ctx_t * ) malloc( sizeof( net_ctx_t ) );

	memset( new_ctx, 0, sizeof( net_ctx_t ) );

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
			free( ctx[i] );
			ctx[i] = NULL;
		}
	}
}

net_ctx_t * net_ctx_find( uint32_t ssrc)
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

				net_ctx_set( ctx[i], ssrc, initiator, send_ssrc, 0, port, ip_address );
				return ctx[i];
			}
		}
	}
	
	return NULL;
}
