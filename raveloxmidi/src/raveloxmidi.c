/*
   raveloxmidi - rtpMIDI implementation for sending NoteOn/Off MIDI events

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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <signal.h>

#include <pthread.h>

#include "net_applemidi.h"
#include "net_listener.h"
#include "midi_journal.h"
#include "net_connection.h"
#include "utils.h"

#include "dns_service_publisher.h"

int main(int argc, char *argv[])
{
	pthread_t dns_publisher_thread;
	char *dns_service_name="raveloxmidi";

	if( net_socket_setup() != 0 )
	{
		fprintf(stderr, "Unable to create sockets\n");
		exit(1);
	}

	net_ctx_init();

        signal( SIGINT , net_socket_loop_shutdown);
        signal( SIGUSR2 , net_socket_loop_shutdown);

	fprintf(stderr, "Starting DNS service publish thread\n");
	
	if( pthread_create( &dns_publisher_thread , NULL, dns_service_publisher, dns_service_name )  )
	{
		fprintf(stderr, "Unable to create publish thread\n");
		exit(1);
	}

	net_socket_loop( 5000 );

	net_socket_destroy();
	net_ctx_destroy();

	return 0;
}
