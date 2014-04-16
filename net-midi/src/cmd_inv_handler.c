#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>

#include <errno.h>
extern int errno;

#include "net_applemidi.h"
#include "net_connection.h"
#include "net_listener.h"

net_response_t * cmd_inv_handler( char *ip_address, uint16_t port, void *data )
{
	net_applemidi_command *cmd = NULL;
	net_applemidi_inv *inv = NULL;
	net_applemidi_inv *accept_inv = NULL;
	net_ctx_t *ctx = NULL;
	net_response_t *response;
	int ret = 0;

	if( ! data ) return NULL;

	inv = ( net_applemidi_inv *) data;

	fprintf(stderr, "INV(%s:%u , ", ip_address, port );

	fprintf(stderr, "name=%s , ", inv->name);
	fprintf(stderr, "ssrc=0x%08x , ", inv->ssrc);
	fprintf(stderr, "version=0x%08x , ", inv->version);
	fprintf(stderr, "initiator=0x%08x )\n", inv->initiator);

	ctx = net_ctx_find_by_ssrc( inv->ssrc );

	if( ! ctx )
	{
		fprintf(stderr, "Registering new connection\n");
		ctx = net_ctx_register( inv->ssrc, inv->initiator, ip_address, port );

		if( ! ctx ) 
		{
			fprintf(stderr, "Error registering connection\n");
		}
	}

	cmd = new_net_applemidi_command( NET_APPLEMIDI_CMD_ACCEPT );

	if( ! cmd )
	{
		fprintf(stderr, "Unable to allocate memory for accept_inv command\n");
		net_ctx_reset( ctx );
		return NULL;
	}

	accept_inv = new_net_applemidi_inv();
	
	if( ! accept_inv ) {
		fprintf(stderr, "Unabled to allocate memory for accept_inv command data\n");
		free( cmd );
		net_ctx_reset( ctx );
		return NULL;
	}

	accept_inv->ssrc = ctx->send_ssrc;
	accept_inv->version = 2;
	accept_inv->initiator = ctx->initiator;
	accept_inv->name = (char *)strdup( "RaveloxMIDI" );

	cmd->data = accept_inv;

	response = new_net_response();

	if( response )
	{
		ret = net_applemidi_pack( cmd , &(response->buffer), &(response->len) );
	}

	net_applemidi_cmd_destroy( &cmd );

	return response;
}
