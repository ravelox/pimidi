#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <signal.h>

#include "net_applemidi.h"
#include "net_listener.h"
#include "midi_journal.h"
#include "net_connection.h"
#include "utils.h"

int main(int argc, char *argv[])
{

	if( net_socket_setup() != 0 )
	{
		fprintf(stderr, "Unable to create sockets\n");
		exit(1);
	}

	net_ctx_init();

        signal( SIGINT , net_socket_loop_shutdown);
        signal( SIGUSR2 , net_socket_loop_shutdown);

	net_socket_loop( 5000 );

	net_socket_destroy();
	net_ctx_destroy();

	return 0;
}
