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

#include "config.h"

#include "net_applemidi.h"
#include "net_socket.h"
#include "net_connection.h"

#include "remote_connection.h"

#include "dns_service_publisher.h"
#include "dns_service_discover.h"

#include "raveloxmidi_config.h"
#include "daemon.h"

#include "logging.h"

#ifdef HAVE_ALSA
#include "raveloxmidi_alsa.h"
#endif

#include "utils.h"

static int running_as_daemon=0;

int main(int argc, char *argv[])
{
	dns_service_desc_t service_desc;
	int ret = 0;

	utils_init();

	config_init( argc, argv);

	if( ! config_is_set("network.bind_address") )
	{
		fprintf( stderr, "No network.bind_address configuration is set\n" );
		goto daemon_stop;
	}

	logging_init();
	logging_printf( LOGGING_INFO, "%s (%s)\n", PACKAGE, VERSION);

	service_desc.name = config_string_get("service.name");
	service_desc.service = "_apple-midi._udp";
	service_desc.port = config_int_get("network.control.port");
	service_desc.publish_ipv4 = is_yes( config_string_get("service.ipv4"));
	service_desc.publish_ipv6 = is_yes( config_string_get("service.ipv6"));

#ifdef HAVE_ALSA
	raveloxmidi_alsa_init( config_string_get("alsa.input_device") , "alsa.output_device" , config_int_get("alsa.input_buffer_size") );
#endif

	if( is_yes( config_string_get("run_as_daemon") ) )
	{
		running_as_daemon = 1;
		daemon_start();
	}

	if( net_socket_init() != 0 )
	{
		logging_printf(LOGGING_ERROR, "Unable to create sockets\n");
		goto daemon_stop;
	}

	net_ctx_init();

        signal( SIGINT , net_socket_loop_shutdown);
        signal( SIGTERM , net_socket_loop_shutdown);
        signal( SIGUSR2 , net_socket_loop_shutdown);

	ret = dns_service_publisher_start( &service_desc );
	
	if( ret != 0 )
	{
		logging_printf(LOGGING_ERROR, "Unable to create publish thread\n");
	} else {
		net_socket_loop_init();

		if( config_string_get("remote.connect") )
		{
			dns_discover_init();
			remote_connect_init();
			dns_discover_teardown();
		}

		if( net_socket_get_shutdown_status() == OK )
		{
#ifdef HAVE_ALSA
			net_socket_alsa_loop();
#endif
			net_socket_fd_loop();
		}
#ifdef HAVE_ALSA
		net_socket_wait_for_alsa();
#endif
		net_socket_loop_teardown();
	}

	dns_service_publisher_stop();

	remote_connect_teardown();
	net_socket_teardown();
	net_ctx_teardown();

#ifdef HAVE_ALSA
	raveloxmidi_alsa_teardown();
#endif

daemon_stop:
	if( running_as_daemon )
	{
		daemon_teardown();
	}


	config_teardown();

	logging_teardown();

	utils_teardown();

	return 0;
}
