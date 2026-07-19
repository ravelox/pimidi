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

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include "raveloxmidi.h"

#include "dns_service_discover.h"
#include "dns_service_publisher.h"
#include "daemon.h"
#include "midi_command.h"
#include "midi_state.h"
#include "midi_sender.h"
#include "net_connection.h"
#include "net_socket.h"
#include "raveloxmidi_config.h"
#include "raveloxmidi_events.h"
#include "remote_connection.h"

#ifdef HAVE_ALSA
#include "raveloxmidi_alsa.h"
#endif

#include "build_info.h"
#include "logging.h"
#include "utils.h"

struct raveloxmidi_context {
	int utils_initialized;
	int config_initialized;
	int logging_initialized;
	int daemon_started;
	int net_socket_initialized;
	int net_ctx_initialized;
	int publisher_started;
	int socket_loop_initialized;
	int midi_sender_initialized;
	int midi_sender_started;
	int remote_connect_initialized;
	int events_initialized;
	int running;
	pthread_t loop_thread;
};

static pthread_mutex_t api_lock = PTHREAD_MUTEX_INITIALIZER;
static raveloxmidi_context_t *active_context = NULL;

static void *raveloxmidi_context_loop( void *data )
{
	(void)data;

#ifdef HAVE_ALSA
	raveloxmidi_alsa_loop();
#endif

	net_socket_fd_loop();

#ifdef HAVE_ALSA
	raveloxmidi_wait_for_alsa();
#endif

	return NULL;
}

static void raveloxmidi_context_lock( void )
{
	pthread_mutex_lock( &api_lock );
}

static void raveloxmidi_context_unlock( void )
{
	pthread_mutex_unlock( &api_lock );
}

static int raveloxmidi_context_runtime_initialized( const raveloxmidi_context_t *context )
{
	if( ! context ) return 0;

	return context->running ||
		context->socket_loop_initialized ||
		context->publisher_started ||
		context->midi_sender_initialized ||
		context->remote_connect_initialized ||
		context->net_socket_initialized ||
		context->net_ctx_initialized;
}

static void raveloxmidi_context_cleanup_runtime( raveloxmidi_context_t *context )
{
	if( ! context ) return;

#ifdef HAVE_ALSA
	raveloxmidi_alsa_teardown();
#endif

	if( context->socket_loop_initialized )
	{
		net_socket_loop_teardown();
		context->socket_loop_initialized = 0;
	}

	if( context->publisher_started )
	{
		dns_service_publisher_stop();
		context->publisher_started = 0;
	}

	if( context->midi_sender_started )
	{
		midi_sender_stop();
		context->midi_sender_started = 0;
	}

	if( context->midi_sender_initialized )
	{
		midi_sender_teardown();
		context->midi_sender_initialized = 0;
	}

	if( context->remote_connect_initialized )
	{
		remote_connect_teardown();
		context->remote_connect_initialized = 0;
	}

	if( context->net_socket_initialized )
	{
		net_socket_teardown();
		context->net_socket_initialized = 0;
	}

	if( context->net_ctx_initialized )
	{
		net_ctx_teardown();
		context->net_ctx_initialized = 0;
	}

	if( context->daemon_started )
	{
		daemon_teardown();
		context->daemon_started = 0;
	}
}

RAVELOXMIDI_API const char *raveloxmidi_version( void )
{
	return VERSION;
}

RAVELOXMIDI_API raveloxmidi_status_t raveloxmidi_context_create( raveloxmidi_context_t **context )
{
	raveloxmidi_context_t *new_context = NULL;
	char *argv[] = { (char *)"raveloxmidi-sdk", NULL };

	if( ! context ) return RAVELOXMIDI_ERROR_INVALID_ARGUMENT;
	if( *context ) return RAVELOXMIDI_ERROR_INVALID_ARGUMENT;

	new_context = ( raveloxmidi_context_t * ) calloc( 1, sizeof( raveloxmidi_context_t ) );
	if( ! new_context ) return RAVELOXMIDI_ERROR_NO_MEMORY;

	raveloxmidi_context_lock();
	if( active_context )
	{
		raveloxmidi_context_unlock();
		free( new_context );
		return RAVELOXMIDI_ERROR_INVALID_STATE;
	}
	active_context = new_context;
	raveloxmidi_context_unlock();

	utils_pthread_tracking_init();
	utils_mem_tracking_init();
	utils_init();
	new_context->utils_initialized = 1;

	config_init( 1, argv );
	new_context->config_initialized = 1;

	if( raveloxmidi_events_init( new_context ) != RAVELOXMIDI_OK )
	{
		config_teardown();
		utils_teardown();
		utils_mem_tracking_teardown();
		utils_pthread_tracking_teardown();
		raveloxmidi_context_lock();
		if( active_context == new_context ) active_context = NULL;
		raveloxmidi_context_unlock();
		free( new_context );
		return RAVELOXMIDI_ERROR_NO_MEMORY;
	}
	new_context->events_initialized = 1;

	*context = new_context;
	return RAVELOXMIDI_OK;
}

RAVELOXMIDI_API raveloxmidi_status_t raveloxmidi_context_set_config( raveloxmidi_context_t *context, const char *key, const char *value )
{
	if( ! context ) return RAVELOXMIDI_ERROR_INVALID_ARGUMENT;
	if( ! key ) return RAVELOXMIDI_ERROR_INVALID_ARGUMENT;
	if( context->running ) return RAVELOXMIDI_ERROR_INVALID_STATE;

	config_add_item( (char *)key, value );
	return RAVELOXMIDI_OK;
}

RAVELOXMIDI_API raveloxmidi_status_t raveloxmidi_context_get_config( raveloxmidi_context_t *context, const char *key, const char **value )
{
	char *config_value = NULL;

	if( ! context ) return RAVELOXMIDI_ERROR_INVALID_ARGUMENT;
	if( ! key ) return RAVELOXMIDI_ERROR_INVALID_ARGUMENT;
	if( ! value ) return RAVELOXMIDI_ERROR_INVALID_ARGUMENT;

	*value = NULL;

	config_value = config_string_get( (char *)key );
	if( ! config_value ) return RAVELOXMIDI_ERROR_NOT_FOUND;

	*value = config_value;
	return RAVELOXMIDI_OK;
}

RAVELOXMIDI_API raveloxmidi_status_t raveloxmidi_context_set_config_file( raveloxmidi_context_t *context, const char *filename )
{
	if( ! context ) return RAVELOXMIDI_ERROR_INVALID_ARGUMENT;
	if( ! filename ) return RAVELOXMIDI_ERROR_INVALID_ARGUMENT;
	if( strlen( filename ) == 0 ) return RAVELOXMIDI_ERROR_INVALID_ARGUMENT;
	if( context->running ) return RAVELOXMIDI_ERROR_INVALID_STATE;

	if( config_load_file( filename ) != 0 ) return RAVELOXMIDI_ERROR;

	config_add_item( "config.file", filename );
	return RAVELOXMIDI_OK;
}

RAVELOXMIDI_API raveloxmidi_status_t raveloxmidi_context_dump_config( raveloxmidi_context_t *context )
{
	if( ! context ) return RAVELOXMIDI_ERROR_INVALID_ARGUMENT;

	if( ! context->logging_initialized )
	{
		logging_init();
		context->logging_initialized = 1;
	}

	config_dump();
	return RAVELOXMIDI_OK;
}

RAVELOXMIDI_API raveloxmidi_status_t raveloxmidi_context_set_midi_event_callback( raveloxmidi_context_t *context, raveloxmidi_midi_event_callback_t callback, void *user_data )
{
	if( ! context ) return RAVELOXMIDI_ERROR_INVALID_ARGUMENT;
	if( ! callback ) return RAVELOXMIDI_ERROR_INVALID_ARGUMENT;

	return raveloxmidi_events_set_callback( context, callback, user_data );
}

RAVELOXMIDI_API raveloxmidi_status_t raveloxmidi_context_clear_midi_event_callback( raveloxmidi_context_t *context )
{
	if( ! context ) return RAVELOXMIDI_ERROR_INVALID_ARGUMENT;

	return raveloxmidi_events_clear_callback( context );
}

RAVELOXMIDI_API raveloxmidi_status_t raveloxmidi_context_send_raw_midi( raveloxmidi_context_t *context, uint8_t status, const uint8_t *data, size_t data_len )
{
	midi_command_t *command = NULL;

	if( ! context ) return RAVELOXMIDI_ERROR_INVALID_ARGUMENT;
	if( data_len > 0 && ! data ) return RAVELOXMIDI_ERROR_INVALID_ARGUMENT;
	if( data_len > 4094 ) return RAVELOXMIDI_ERROR_INVALID_ARGUMENT;
	if( ! context->midi_sender_started ) return RAVELOXMIDI_ERROR_NOT_RUNNING;

	command = midi_command_create();
	if( ! command ) return RAVELOXMIDI_ERROR_NO_MEMORY;

	midi_command_set( command, 0, status, data, data_len );
	if( data_len > 0 && ! command->data )
	{
		midi_command_destroy( (void **)&command );
		return RAVELOXMIDI_ERROR_NO_MEMORY;
	}

	midi_sender_add( command, NULL );

	return RAVELOXMIDI_OK;
}

RAVELOXMIDI_API raveloxmidi_status_t raveloxmidi_context_start( raveloxmidi_context_t *context )
{
	dns_service_desc_t service_desc;
	int err = 0;

	if( ! context ) return RAVELOXMIDI_ERROR_INVALID_ARGUMENT;
	if( context->running ) return RAVELOXMIDI_ERROR_INVALID_STATE;
	if( ! config_is_set( "network.bind_address" ) ) return RAVELOXMIDI_ERROR_INVALID_ARGUMENT;

	if( ! context->logging_initialized )
	{
		logging_init();
		context->logging_initialized = 1;
	}

	logging_printf( LOGGING_INFO, "%s (%s-%s)\n", PACKAGE, VERSION, GIT_BRANCH_NAME );

	memset( &service_desc, 0, sizeof( service_desc ) );
	service_desc.name = config_string_get( "service.name" );
	service_desc.service = "_apple-midi._udp";
	service_desc.port = config_int_get( "network.control.port" );
	service_desc.publish_ipv4 = is_yes( config_string_get( "service.ipv4" ) );
	service_desc.publish_ipv6 = is_yes( config_string_get( "service.ipv6" ) );

	if( is_yes( config_string_get( "run_as_daemon" ) ) )
	{
		daemon_start();
		context->daemon_started = 1;
	}

	context->net_socket_initialized = 1;
	if( net_socket_init() != 0 )
	{
		raveloxmidi_context_cleanup_runtime( context );
		return RAVELOXMIDI_ERROR;
	}

#ifdef HAVE_ALSA
	raveloxmidi_alsa_init( "alsa.input_device", "alsa.output_device", config_int_get( "alsa.input_buffer_size" ) );
#endif

	net_ctx_init();
	context->net_ctx_initialized = 1;

	if( dns_service_publisher_start( &service_desc ) != 0 ) goto start_error;
	context->publisher_started = 1;

	net_socket_loop_init();
	context->socket_loop_initialized = 1;

	midi_sender_init();
	context->midi_sender_initialized = 1;

	midi_sender_start();
	context->midi_sender_started = 1;

	if( config_string_get( "remote.connect" ) )
	{
		dns_discover_init();
		remote_connect_init();
		dns_discover_teardown();
		context->remote_connect_initialized = 1;
	}

	err = pthread_create( &context->loop_thread, NULL, raveloxmidi_context_loop, context );
	if( err != 0 ) goto start_error;

	context->running = 1;
	return RAVELOXMIDI_OK;

start_error:
	raveloxmidi_context_cleanup_runtime( context );
	return RAVELOXMIDI_ERROR;
}

RAVELOXMIDI_API raveloxmidi_status_t raveloxmidi_context_request_stop( raveloxmidi_context_t *context )
{
	if( ! context ) return RAVELOXMIDI_ERROR_INVALID_ARGUMENT;

	if( ! raveloxmidi_context_runtime_initialized( context ) ) return RAVELOXMIDI_ERROR_NOT_RUNNING;

	if( context->socket_loop_initialized ) net_socket_loop_shutdown( 0 );

	return RAVELOXMIDI_OK;
}

RAVELOXMIDI_API raveloxmidi_status_t raveloxmidi_context_wait( raveloxmidi_context_t *context )
{
	if( ! context ) return RAVELOXMIDI_ERROR_INVALID_ARGUMENT;

	if( ! raveloxmidi_context_runtime_initialized( context ) ) return RAVELOXMIDI_ERROR_NOT_RUNNING;

	if( context->running )
	{
		pthread_join( context->loop_thread, NULL );
		context->running = 0;
	}

	raveloxmidi_context_cleanup_runtime( context );

	return RAVELOXMIDI_OK;
}

RAVELOXMIDI_API raveloxmidi_status_t raveloxmidi_context_stop( raveloxmidi_context_t *context )
{
	raveloxmidi_status_t status = RAVELOXMIDI_OK;

	if( ! context ) return RAVELOXMIDI_ERROR_INVALID_ARGUMENT;

	status = raveloxmidi_context_request_stop( context );
	if( status != RAVELOXMIDI_OK ) return status;

	return raveloxmidi_context_wait( context );
}

RAVELOXMIDI_API void raveloxmidi_context_free( raveloxmidi_context_t **context )
{
	raveloxmidi_context_t *old_context = NULL;

	if( ! context ) return;
	if( ! *context ) return;

	old_context = *context;

	if( raveloxmidi_context_runtime_initialized( old_context ) ) raveloxmidi_context_stop( old_context );

	if( old_context->events_initialized )
	{
		raveloxmidi_events_teardown( old_context );
		old_context->events_initialized = 0;
	}

	if( old_context->logging_initialized )
	{
		logging_teardown();
		old_context->logging_initialized = 0;
	}

	if( old_context->config_initialized )
	{
		config_teardown();
		old_context->config_initialized = 0;
	}

	if( old_context->utils_initialized )
	{
		utils_teardown();
		utils_mem_tracking_teardown();
		utils_pthread_tracking_teardown();
		old_context->utils_initialized = 0;
	}

	raveloxmidi_context_lock();
	if( active_context == old_context ) active_context = NULL;
	raveloxmidi_context_unlock();

	free( old_context );
	*context = NULL;
}
