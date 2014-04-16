#ifndef NET_CONNECTION_H
#define NET_CONNECTION_H

#include "midi_journal.h"

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
	journal_t	*journal;
} net_ctx_t;



void net_ctx_reset( net_ctx_t *ctx );
void net_ctx_init( void );
void net_ctx_destroy( void );
net_ctx_t * net_ctx_find_by_ssrc( uint32_t ssrc);
net_ctx_t * net_ctx_find_by_id( uint8_t id);
net_ctx_t * net_ctx_register( uint32_t ssrc, uint32_t initiator, char *ip_address, uint16_t port );
void net_ctx_add_journal_note( uint8_t ctx_id , char channel, char note, char velocity );
void debug_ctx_journal_dump( uint8_t ctx_id );
void net_ctx_journal_pack( uint8_t ctx_id, char **buffer, size_t *buffer_len);
void net_ctx_journal_reset( uint8_t ctx_id);

#endif
