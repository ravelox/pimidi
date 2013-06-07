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
	net_socket_setup();
	net_ctx_init();

        signal( SIGINT , net_socket_loop_shutdown);
        signal( SIGUSR2 , net_socket_loop_shutdown);

	net_socket_loop( 5000 );

	net_socket_destroy();
	net_ctx_destroy();

	return 0;
}
