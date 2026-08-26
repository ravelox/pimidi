/*
   This file is part of raveloxmidi.

   Copyright (C) 2026 Dave Kelly

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

/*
	Exercises the STAT/QUIT/LIST command interface on its own
	network.command.port socket, and confirms the old local MIDI
	port (network.local.port) no longer answers those commands.
*/

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "raveloxmidi_config.h"
#include "net_socket.h"
#include "net_connection.h"
#include "utils.h"

#define TEST_CONTROL_PORT 19040
#define TEST_DATA_PORT 19041
#define TEST_LOCAL_PORT 19042
#define TEST_COMMAND_PORT 19043

static void *loop_thread_fn( void *arg )
{
	(void)arg;
	net_socket_fd_loop();
	return NULL;
}

/* Sends payload to 127.0.0.1:port and waits up to timeout_ms for a reply.
   Returns the number of bytes received, or -1 if nothing arrived in time. */
static ssize_t udp_roundtrip( unsigned int port, const void *payload, size_t payload_len,
	unsigned char *response, size_t response_cap, int timeout_ms )
{
	int sock = -1;
	struct sockaddr_in addr;
	struct timeval tv;
	ssize_t received = -1;

	sock = socket( AF_INET, SOCK_DGRAM, 0 );
	if( sock < 0 ) return -1;

	memset( &addr, 0, sizeof( addr ) );
	addr.sin_family = AF_INET;
	addr.sin_port = htons( (uint16_t)port );
	inet_pton( AF_INET, "127.0.0.1", &addr.sin_addr );

	tv.tv_sec = timeout_ms / 1000;
	tv.tv_usec = ( timeout_ms % 1000 ) * 1000;
	setsockopt( sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof( tv ) );

	if( connect( sock, (struct sockaddr *)&addr, sizeof( addr ) ) < 0 )
	{
		close( sock );
		return -1;
	}

	if( send( sock, payload, payload_len, 0 ) < 0 )
	{
		close( sock );
		return -1;
	}

	received = recv( sock, response, response_cap, 0 );

	close( sock );

	return received;
}

static int expect_reply( const char *label, unsigned int port, const char *command, const char *expected )
{
	unsigned char response[256];
	ssize_t received;

	memset( response, 0, sizeof( response ) );
	received = udp_roundtrip( port, command, strlen( command ), response, sizeof( response ) - 1, 2000 );

	if( received < 0 )
	{
		fprintf( stderr, "%s: timed out waiting for a reply\n", label );
		return 1;
	}

	if( strncmp( (char *)response, expected, strlen( expected ) ) != 0 )
	{
		fprintf( stderr, "%s: expected reply starting with \"%s\", got \"%s\"\n", label, expected, response );
		return 1;
	}

	return 0;
}

static int expect_silence( const char *label, unsigned int port, const char *command )
{
	unsigned char response[256];
	ssize_t received;

	memset( response, 0, sizeof( response ) );
	received = udp_roundtrip( port, command, strlen( command ), response, sizeof( response ) - 1, 300 );

	if( received >= 0 )
	{
		fprintf( stderr, "%s: expected no reply, got %zd bytes: \"%s\"\n", label, received, response );
		return 1;
	}

	return 0;
}

int main( void )
{
	char *argv[] = { (char *)"net_socket_test", NULL };
	pthread_t loop_thread;
	int failed = 0;

	utils_pthread_tracking_init();
	utils_mem_tracking_init();
	utils_init();

	config_init( 1, argv );
	config_add_item( "network.bind_address", "127.0.0.1" );
	config_add_item( "network.control.port", "19040" );
	config_add_item( "network.data.port", "19041" );
	config_add_item( "network.local.port", "19042" );
	config_add_item( "network.command.port", "19043" );
	config_add_item( "service.ipv4", "no" );
	config_add_item( "service.ipv6", "no" );

	if( net_socket_init() != 0 )
	{
		fprintf( stderr, "net_socket_init failed\n" );
		return 1;
	}

	net_ctx_init();
	net_socket_loop_init();

	if( pthread_create( &loop_thread, NULL, loop_thread_fn, NULL ) != 0 )
	{
		fprintf( stderr, "unable to start socket loop thread\n" );
		return 1;
	}

	/* Give the poll loop a moment to start waiting on the listener sockets. */
	usleep( 100000 );

	/* STAT/LIST answer on the new command port. */
	failed |= expect_reply( "STAT on command port", TEST_COMMAND_PORT, "STAT", "OK" );
	failed |= expect_reply( "LIST on command port", TEST_COMMAND_PORT, "LIST", "{\"connections\":[]" );

	/* Unrecognized traffic on the command port is dropped, not echoed or crashed on. */
	failed |= expect_silence( "unrecognized command", TEST_COMMAND_PORT, "ZZZZ" );

	/* The old local MIDI port no longer answers commands - this is the breaking change. */
	failed |= expect_silence( "STAT on local port", TEST_LOCAL_PORT, "STAT" );
	failed |= expect_silence( "LIST on local port", TEST_LOCAL_PORT, "LIST" );

	/* QUIT still shuts the daemon down, now via the command port. */
	failed |= expect_reply( "QUIT on command port", TEST_COMMAND_PORT, "QUIT", "QT" );

	pthread_join( loop_thread, NULL );

	if( net_socket_get_shutdown_status() != SHUTDOWN )
	{
		fprintf( stderr, "QUIT on command port did not signal shutdown\n" );
		failed = 1;
	}

	net_socket_loop_teardown();
	net_socket_teardown();
	net_ctx_teardown();
	config_teardown();
	utils_teardown();
	utils_mem_tracking_teardown();
	utils_pthread_tracking_teardown();

	return failed;
}
