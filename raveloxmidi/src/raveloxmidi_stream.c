/*
   raveloxmidi-stream - raw MIDI stream bridge for libraveloxmidi

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

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "raveloxmidi.h"

typedef struct stream_output_item_t {
	uint8_t *data;
	size_t len;
	struct stream_output_item_t *next;
} stream_output_item_t;

typedef struct stream_output_queue_t {
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	stream_output_item_t *head;
	stream_output_item_t *tail;
	const char *path;
	int fd;
	int enabled;
	int stop;
	pthread_t thread;
} stream_output_queue_t;

typedef struct stream_input_t {
	raveloxmidi_context_t *context;
	const char *path;
	int enabled;
	int stop_when_done;
	pthread_t thread;
} stream_input_t;

typedef struct stream_app_t {
	raveloxmidi_context_t *context;
	stream_input_t input;
	stream_output_queue_t output;
	volatile sig_atomic_t stop_requested;
} stream_app_t;

static stream_app_t *signal_app = NULL;

static void usage( void )
{
	fprintf( stderr, "Usage:\n" );
	fprintf( stderr, "\traveloxmidi-stream --bind-address address [--input -|path] [--output -|path] [options]\n" );
	fprintf( stderr, "\traveloxmidi-stream --config filename [--input -|path] [--output -|path] [options]\n" );
	fprintf( stderr, "\nOptions:\n" );
	fprintf( stderr, "\t-c, --config filename       Load a raveloxmidi config file\n" );
	fprintf( stderr, "\t-b, --bind-address address  Set network.bind_address\n" );
	fprintf( stderr, "\t-i, --input path            Read raw MIDI from path, or '-' for stdin\n" );
	fprintf( stderr, "\t-o, --output path           Write received raw MIDI to path, or '-' for stdout\n" );
	fprintf( stderr, "\t-d, --debug                 Enable debug logging\n" );
	fprintf( stderr, "\t-v, --version               Print version\n" );
	fprintf( stderr, "\t-h, --help                  Show this help\n" );
}

static void shutdown_handler( int sig )
{
	(void)sig;

	if( signal_app )
	{
		signal_app->stop_requested = 1;
		if( signal_app->context ) raveloxmidi_context_request_stop( signal_app->context );
	}
}

static int status_is_ok( const char *operation, raveloxmidi_status_t status )
{
	if( status == RAVELOXMIDI_OK ) return 1;

	fprintf( stderr, "%s failed: %d\n", operation, status );
	return 0;
}

static int set_config_or_fail( raveloxmidi_context_t *context, const char *key, const char *value )
{
	return status_is_ok( key, raveloxmidi_context_set_config( context, key, value ) );
}

static size_t midi_data_len_for_status( uint8_t status )
{
	uint8_t type = status & 0xf0;

	if( status < 0x80 ) return 0;
	if( status < 0xf0 )
	{
		if( type == 0xc0 || type == 0xd0 ) return 1;
		return 2;
	}

	switch( status )
	{
		case 0xf1:
		case 0xf3:
			return 1;
		case 0xf2:
			return 2;
		default:
			return 0;
	}
}

static int wait_for_fd( int fd, short events, volatile sig_atomic_t *stop_requested )
{
	while( ! *stop_requested )
	{
		struct pollfd pfd;
		int ret = 0;

		pfd.fd = fd;
		pfd.events = events;
		pfd.revents = 0;

		ret = poll( &pfd, 1, 250 );
		if( ret > 0 )
		{
			if( pfd.revents & ( POLLERR | POLLNVAL ) ) return -1;
			if( pfd.revents & events ) return 1;
			if( pfd.revents & POLLHUP ) return 0;
		}
		else if( ret < 0 && errno != EINTR )
		{
			return -1;
		}
	}

	return -2;
}

static int read_exact( int fd, uint8_t *buffer, size_t len, volatile sig_atomic_t *stop_requested )
{
	size_t total = 0;

	while( total < len )
	{
		int wait_result = wait_for_fd( fd, POLLIN, stop_requested );
		ssize_t nread = 0;

		if( wait_result <= 0 ) return wait_result;
		nread = read( fd, buffer + total, len - total );
		if( nread == 0 ) return 0;
		if( nread < 0 )
		{
			if( errno == EINTR ) continue;
			if( errno == EAGAIN || errno == EWOULDBLOCK ) continue;
			return -1;
		}
		total += (size_t)nread;
	}

	return 1;
}

static int write_exact( int fd, const uint8_t *buffer, size_t len, volatile sig_atomic_t *stop_requested )
{
	size_t total = 0;

	while( total < len )
	{
		int wait_result = wait_for_fd( fd, POLLOUT, stop_requested );
		ssize_t nwritten = 0;

		if( wait_result <= 0 ) return wait_result == -2 ? 0 : -1;
		nwritten = write( fd, buffer + total, len - total );
		if( nwritten < 0 )
		{
			if( errno == EINTR ) continue;
			if( errno == EAGAIN || errno == EWOULDBLOCK ) continue;
			return -1;
		}
		total += (size_t)nwritten;
	}

	return 0;
}

static int open_input_fd( const char *path )
{
	struct stat st;

	if( ! path || strcmp( path, "-" ) == 0 ) return STDIN_FILENO;
	if( stat( path, &st ) == 0 && S_ISFIFO( st.st_mode ) ) return open( path, O_RDWR | O_NONBLOCK );

	return open( path, O_RDONLY );
}

static int open_output_fd( const char *path, volatile sig_atomic_t *stop_requested )
{
	struct stat st;
	int flags = O_WRONLY | O_CREAT | O_TRUNC;

	if( ! path || strcmp( path, "-" ) == 0 ) return STDOUT_FILENO;

	if( stat( path, &st ) == 0 && S_ISFIFO( st.st_mode ) ) flags = O_WRONLY | O_NONBLOCK;

	while( ! *stop_requested )
	{
		int fd = open( path, flags, 0666 );
		if( fd >= 0 ) return fd;
		if( errno != ENXIO && errno != EINTR ) return -1;
		sleep( 1 );
	}

	errno = EINTR;
	return -1;
}

static void output_queue_init( stream_output_queue_t *queue )
{
	memset( queue, 0, sizeof( *queue ) );
	pthread_mutex_init( &queue->mutex, NULL );
	pthread_cond_init( &queue->cond, NULL );
	queue->fd = -1;
}

static void output_queue_destroy_items( stream_output_queue_t *queue )
{
	stream_output_item_t *item = queue->head;

	while( item )
	{
		stream_output_item_t *next = item->next;
		free( item->data );
		free( item );
		item = next;
	}

	queue->head = NULL;
	queue->tail = NULL;
}

static void output_queue_teardown( stream_output_queue_t *queue )
{
	output_queue_destroy_items( queue );
	pthread_cond_destroy( &queue->cond );
	pthread_mutex_destroy( &queue->mutex );
}

static int output_queue_push( stream_output_queue_t *queue, const uint8_t *data, size_t len )
{
	stream_output_item_t *item = NULL;

	if( ! queue->enabled ) return 0;
	if( ! data || len == 0 ) return 0;

	item = (stream_output_item_t *)calloc( 1, sizeof( *item ) );
	if( ! item ) return -1;

	item->data = (uint8_t *)malloc( len );
	if( ! item->data )
	{
		free( item );
		return -1;
	}

	memcpy( item->data, data, len );
	item->len = len;

	pthread_mutex_lock( &queue->mutex );
	if( queue->tail ) queue->tail->next = item;
	else queue->head = item;
	queue->tail = item;
	pthread_cond_signal( &queue->cond );
	pthread_mutex_unlock( &queue->mutex );

	return 0;
}

static stream_output_item_t *output_queue_pop( stream_output_queue_t *queue )
{
	stream_output_item_t *item = NULL;

	pthread_mutex_lock( &queue->mutex );
	while( ! queue->stop && ! queue->head )
	{
		pthread_cond_wait( &queue->cond, &queue->mutex );
	}

	if( queue->head )
	{
		item = queue->head;
		queue->head = item->next;
		if( ! queue->head ) queue->tail = NULL;
	}
	pthread_mutex_unlock( &queue->mutex );

	return item;
}

static void output_queue_stop( stream_output_queue_t *queue )
{
	pthread_mutex_lock( &queue->mutex );
	queue->stop = 1;
	pthread_cond_signal( &queue->cond );
	pthread_mutex_unlock( &queue->mutex );
}

static void *output_thread_main( void *data )
{
	stream_app_t *app = (stream_app_t *)data;
	stream_output_queue_t *queue = &app->output;

	queue->fd = open_output_fd( queue->path, &app->stop_requested );
	if( queue->fd < 0 )
	{
		perror( "open output" );
		app->stop_requested = 1;
		raveloxmidi_context_request_stop( app->context );
		return NULL;
	}

	while( ! app->stop_requested )
	{
		stream_output_item_t *item = output_queue_pop( queue );
		if( ! item )
		{
			if( queue->stop ) break;
			continue;
		}

		if( write_exact( queue->fd, item->data, item->len, &app->stop_requested ) != 0 )
		{
			perror( "write output" );
			app->stop_requested = 1;
			raveloxmidi_context_request_stop( app->context );
		}

		free( item->data );
		free( item );
	}

	if( queue->fd >= 0 && queue->fd != STDOUT_FILENO ) close( queue->fd );
	queue->fd = -1;

	return NULL;
}

static void midi_event_callback( raveloxmidi_context_t *context, const raveloxmidi_midi_event_t *event, void *user_data )
{
	stream_app_t *app = (stream_app_t *)user_data;
	uint8_t buffer[4];

	(void)context;

	if( ! app || ! event ) return;
	if( event->data_len > 3 ) return;

	buffer[0] = event->status;
	if( event->data_len > 0 ) memcpy( buffer + 1, event->data, event->data_len );

	if( output_queue_push( &app->output, buffer, event->data_len + 1 ) != 0 )
	{
		app->stop_requested = 1;
		raveloxmidi_context_request_stop( app->context );
	}
}

static void *input_thread_main( void *data )
{
	stream_app_t *app = (stream_app_t *)data;
	stream_input_t *input = &app->input;
	int fd = -1;

	fd = open_input_fd( input->path );
	if( fd < 0 )
	{
		perror( "open input" );
		app->stop_requested = 1;
		raveloxmidi_context_request_stop( app->context );
		return NULL;
	}

	while( ! app->stop_requested )
	{
		uint8_t status = 0;
		uint8_t data_bytes[2];
		size_t data_len = 0;
		int read_result = read_exact( fd, &status, 1, &app->stop_requested );

		if( read_result == 0 ) break;
		if( read_result < 0 )
		{
			perror( "read status" );
			app->stop_requested = 1;
			break;
		}

		data_len = midi_data_len_for_status( status );
		if( data_len == 0 && status < 0x80 )
		{
			fprintf( stderr, "Ignoring MIDI data byte without status: 0x%02x\n", status );
			continue;
		}

		if( data_len > 0 )
		{
			read_result = read_exact( fd, data_bytes, data_len, &app->stop_requested );
			if( read_result == 0 ) break;
			if( read_result < 0 )
			{
				perror( "read data" );
				app->stop_requested = 1;
				break;
			}
		}

		if( raveloxmidi_context_send_raw_midi( input->context, status, data_bytes, data_len ) != RAVELOXMIDI_OK )
		{
			fprintf( stderr, "Unable to send MIDI message\n" );
			app->stop_requested = 1;
			break;
		}
	}

	if( fd >= 0 && fd != STDIN_FILENO ) close( fd );

	if( input->stop_when_done )
	{
		app->stop_requested = 1;
		raveloxmidi_context_request_stop( app->context );
	}

	return NULL;
}

int main( int argc, char **argv )
{
	stream_app_t app;
	raveloxmidi_status_t status = RAVELOXMIDI_OK;
	const char *bind_address = NULL;
	int rc = EXIT_SUCCESS;
	int input_started = 0;
	int output_started = 0;
	static struct option long_options[] = {
		{"config", required_argument, NULL, 'c'},
		{"bind-address", required_argument, NULL, 'b'},
		{"input", required_argument, NULL, 'i'},
		{"output", required_argument, NULL, 'o'},
		{"debug", no_argument, NULL, 'd'},
		{"version", no_argument, NULL, 'v'},
		{"help", no_argument, NULL, 'h'},
		{0, 0, 0, 0}
	};

	memset( &app, 0, sizeof( app ) );
	output_queue_init( &app.output );

	status = raveloxmidi_context_create( &app.context );
	if( status != RAVELOXMIDI_OK )
	{
		fprintf( stderr, "Unable to create raveloxmidi context\n" );
		rc = EXIT_FAILURE;
		goto cleanup;
	}

	while( 1 )
	{
		int c = getopt_long( argc, argv, "c:b:i:o:dvh", long_options, NULL );
		if( c == -1 ) break;

		switch( c )
		{
			case 'c':
				if( ! status_is_ok( "load config", raveloxmidi_context_set_config_file( app.context, optarg ) ) )
				{
					rc = EXIT_FAILURE;
					goto cleanup;
				}
				break;
			case 'b':
				bind_address = optarg;
				break;
			case 'i':
				app.input.enabled = 1;
				app.input.path = optarg;
				break;
			case 'o':
				app.output.enabled = 1;
				app.output.path = optarg;
				break;
			case 'd':
				if( ! set_config_or_fail( app.context, "logging.enabled", "yes" ) ||
					! set_config_or_fail( app.context, "logging.log_level", "debug" ) )
				{
					rc = EXIT_FAILURE;
					goto cleanup;
				}
				break;
			case 'v':
				fprintf( stderr, "raveloxmidi-stream (%s)\n", raveloxmidi_version() );
				goto cleanup;
			case 'h':
				usage();
				goto cleanup;
			default:
				usage();
				rc = EXIT_FAILURE;
				goto cleanup;
		}
	}

	if( ! app.input.enabled && ! app.output.enabled )
	{
		fprintf( stderr, "At least one of --input or --output is required\n" );
		usage();
		rc = EXIT_FAILURE;
		goto cleanup;
	}

	if( bind_address && ! set_config_or_fail( app.context, "network.bind_address", bind_address ) )
	{
		rc = EXIT_FAILURE;
		goto cleanup;
	}

	if( ! set_config_or_fail( app.context, "run_as_daemon", "no" ) ||
		! set_config_or_fail( app.context, "inbound_midi", "none" ) )
	{
		rc = EXIT_FAILURE;
		goto cleanup;
	}

	if( app.output.enabled )
	{
		status = raveloxmidi_context_set_midi_event_callback( app.context, midi_event_callback, &app );
		if( ! status_is_ok( "set MIDI event callback", status ) )
		{
			rc = EXIT_FAILURE;
			goto cleanup;
		}
	}

	signal_app = &app;
	signal( SIGINT, shutdown_handler );
	signal( SIGTERM, shutdown_handler );

	status = raveloxmidi_context_start( app.context );
	if( ! status_is_ok( "start context", status ) )
	{
		rc = EXIT_FAILURE;
		goto cleanup;
	}

	if( app.output.enabled )
	{
		if( pthread_create( &app.output.thread, NULL, output_thread_main, &app ) != 0 )
		{
			perror( "create output thread" );
			rc = EXIT_FAILURE;
			goto cleanup;
		}
		output_started = 1;
	}

	if( app.input.enabled )
	{
		app.input.context = app.context;
		app.input.stop_when_done = ! app.output.enabled;
		if( pthread_create( &app.input.thread, NULL, input_thread_main, &app ) != 0 )
		{
			perror( "create input thread" );
			rc = EXIT_FAILURE;
			goto cleanup;
		}
		input_started = 1;
	}

	status = raveloxmidi_context_wait( app.context );
	if( status != RAVELOXMIDI_OK && ! app.stop_requested )
	{
		fprintf( stderr, "Unable to wait for raveloxmidi shutdown\n" );
		rc = EXIT_FAILURE;
	}

cleanup:
	app.stop_requested = 1;
	if( app.context ) raveloxmidi_context_request_stop( app.context );
	if( input_started ) pthread_join( app.input.thread, NULL );
	output_queue_stop( &app.output );
	if( output_started ) pthread_join( app.output.thread, NULL );
	signal_app = NULL;
	raveloxmidi_context_free( &app.context );
	output_queue_teardown( &app.output );

	return rc;
}
