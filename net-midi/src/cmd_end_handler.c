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

net_response_t * cmd_end_handler( void *data )
{
	net_applemidi_inv *inv = NULL;
	net_ctx_t *ctx = NULL;

	if( ! data ) return NULL;

	inv = ( net_applemidi_inv *) data;

	fprintf(stderr, "BYE( ");

	fprintf(stderr, "name=<<%s>> , ", inv->name);
	fprintf(stderr, "ssrc=0x%08x , ", inv->ssrc);
	fprintf(stderr, "version = 0x%08x , ", inv->version);
	fprintf(stderr, "initiator=0x%08x )\n", inv->initiator);

	ctx = net_ctx_find_by_ssrc( inv->ssrc );

	if( ! ctx )
	{
		fprintf(stderr, "No existing connection found\n");
		return NULL;
	}

	net_ctx_reset( ctx );

	return NULL;
}
