#ifndef NET_LISTENER_H
#define NET_LISTENER_H

typedef struct net_response_t {
	unsigned char *buffer;
	size_t len;
} net_response_t;

net_response_t * new_net_response( void );
void net_response_destroy( net_response_t **response );

int net_socket_create( unsigned int port );
int net_socket_destroy( void );
int net_socket_listener( void );
int net_socket_loop( unsigned int interval );
void net_socket_loop_shutdown(int signal);
int net_socket_setup( void );

#endif
