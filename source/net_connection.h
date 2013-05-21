#ifndef NET_CONNECTION_H
#define NET_CONNECTION_H

#define MAX_CTX 10

typedef struct net_ctx_t {
	uint8_t		used;
	uint32_t	ssrc;
	uint32_t	send_ssrc;
	uint32_t	initiator;
	uint32_t	seq;
	uint16_t	port;
	time_t		start;
	char * 		ip_address;
	
} net_ctx_t;


void net_ctx_reset( net_ctx_t *ctx );
void net_ctx_init( void );
void net_ctx_destroy( void );
net_ctx_t * net_ctx_register( uint32_t ssrc, uint32_t initiator, char *ip_address, uint16_t port );
net_ctx_t * net_ctx_find( uint32_t ssrc);

#endif
