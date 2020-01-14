/*
   This file is part of raveloxmidi.

   Copyright (C) 2018 Dave Kelly

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
#include "config.h"

#ifdef HAVE_ALSA

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include <asoundlib.h>

#include <signal.h>
#include <pthread.h>

#include "net_socket.h"
#include "raveloxmidi_alsa.h"
#include "utils.h"
#include "logging.h"
#include "raveloxmidi_config.h"

/* Table of ALSA output handles */
static snd_rawmidi_t **outputs = NULL;
static int num_outputs = 0;
static pthread_mutex_t output_table_lock;

/* Table of ALSA input handles */
static snd_rawmidi_t **inputs = NULL;
static int num_inputs = 0;
static pthread_mutex_t input_table_lock;

/* Table of input file descriptors for polling */
static int num_poll_descriptors = 0;
static struct pollfd *poll_descriptors = NULL;
static pthread_mutex_t poll_table_lock;

static pthread_t alsa_listener_thread;

extern int errno;

/* Open an output handle for the specified ALSA device and add it to the list */
static void raveloxmidi_alsa_add_output( char *device_name )
{
	snd_rawmidi_t *output_handle = NULL;
	snd_rawmidi_t **new_outputs = NULL;
	int ret = 0;

	if( ! device_name ) return;

	ret = snd_rawmidi_open( NULL, &output_handle, device_name, SND_RAWMIDI_NONBLOCK );
	logging_printf(LOGGING_DEBUG,"raveloxmidi_alsa_init output: device=%s ret=%d %s\n", device_name, ret, snd_strerror( ret ) );
	if( ret != 0 ) return;

	raveloxmidi_alsa_dump_rawmidi( output_handle );

	new_outputs = (snd_rawmidi_t **)realloc( outputs , ( num_outputs + 1 ) * sizeof(snd_rawmidi_t *) );

	if(! new_outputs )
	{
		/* Close the open handle */
		raveloxmidi_alsa_handle_destroy( output_handle );
		logging_printf( LOGGING_ERROR, "raveloxmidi_alsa_add_output: Insufficient memory for new item: %s\n", device_name);
		return;
	}

	outputs = new_outputs;
	outputs[ num_outputs++ ] = output_handle;
}

/* Open an input handle for the specified ALSA device and add it to the list */
static void raveloxmidi_alsa_add_input( char *device_name , size_t buffer_size)
{
	snd_rawmidi_t *input_handle = NULL;
	snd_rawmidi_t **new_inputs = NULL;
	int ret = 0;

	if( ! device_name ) return;

	ret = snd_rawmidi_open( &input_handle, NULL, device_name, SND_RAWMIDI_NONBLOCK );
	logging_printf(LOGGING_DEBUG,"raveloxmidi_alsa_init input: device=%s ret=%d %s\n", device_name, ret, snd_strerror( ret ) );

	if( ret != 0 ) return;

	/* Set the input buffer size */
	if( (buffer_size > RAVELOXMIDI_ALSA_DEFAULT_BUFFER) && (buffer_size <= RAVELOXMIDI_ALSA_MAX_BUFFER) )
	{
		snd_rawmidi_params_t *params = NULL;
		snd_rawmidi_params_malloc( &params );
		snd_rawmidi_params_current( input_handle, params );
		snd_rawmidi_params_set_buffer_size( input_handle, params, buffer_size );
		snd_rawmidi_params( input_handle, params );
		snd_rawmidi_params_free( params );
		free( params );
	}

	raveloxmidi_alsa_dump_rawmidi( input_handle );

	new_inputs = (snd_rawmidi_t **)realloc( inputs, ( num_inputs + 1 ) * sizeof( snd_rawmidi_t * ) );
	if( ! new_inputs )
	{
		
		/* Close the open handle */
		raveloxmidi_alsa_handle_destroy( input_handle );
		logging_printf( LOGGING_ERROR, "raveloxmidi_alsa_add_input: Insufficient memory for new item: %s\n", device_name);
		return;
	}

	inputs = new_inputs;
	inputs[ num_inputs++ ] = input_handle;

	/* Add the file descriptors to the polling table */
	raveloxmidi_alsa_set_poll_fds( input_handle );
}

void raveloxmidi_alsa_init( char *input_name , char *output_name , size_t buffer_size)
{
	int ret = 0;

	outputs = NULL;
	num_outputs = 0;

	inputs = NULL;
	num_inputs = 0;

	pthread_mutex_init( &output_table_lock , NULL);
	pthread_mutex_init( &input_table_lock , NULL);
	pthread_mutex_init( &poll_table_lock , NULL);

	if( input_name )
	{
		raveloxmidi_config_iter_t *input_key = NULL;

		/* Look for config item by literal name */
		if( config_is_set( input_name ) )
		{
			raveloxmidi_alsa_add_input( config_string_get( input_name ) , buffer_size );
		}
	
		/* Now look for multiple config items */
		input_key = config_iter_create( input_name );
		while(1)
		{
			char *full_input_name = NULL;
			if( ! config_iter_is_set( input_key ) ) break;

			full_input_name = config_iter_string_get( input_key );

			if( full_input_name )
			{
				raveloxmidi_alsa_add_input( full_input_name , buffer_size );
			}

			config_iter_next( input_key );
		}
		config_iter_destroy( &input_key );
	}

	if( output_name )
	{
		raveloxmidi_config_iter_t *output_key = NULL;

		/* Look for config item by literal name */
		if( config_is_set( output_name ) )
		{
			raveloxmidi_alsa_add_output( config_string_get( output_name ) );
		}
	
		/* Now look for multiple config items */
		output_key = config_iter_create( output_name );
		while(1)
		{
			char *full_output_name = NULL;
			if( ! config_iter_is_set( output_key ) ) break;

			full_output_name = config_iter_string_get( output_key );

			if( full_output_name )
			{
				raveloxmidi_alsa_add_output( full_output_name );
			}

			config_iter_next( output_key );
		}
		config_iter_destroy( &output_key );
	}
}

void raveloxmidi_alsa_handle_destroy( snd_rawmidi_t *rawmidi )
{
	int status = 0;

	if( ! rawmidi ) return;

	status = snd_rawmidi_drain( rawmidi );
	if( status < 0 )
	{
		logging_printf(LOGGING_WARN, "raveloxmidi_alsa_handle_destroy: snd_rawmidi_drain()=%d : %s\n", status, snd_strerror(status) );
	}

	status = snd_rawmidi_close( rawmidi );
	if( status < 0 )
	{
		logging_printf(LOGGING_WARN, "raveloxmidi_alsa_handle_destroy: snd_rawmidi_close()=%d : %s\n", status, snd_strerror(status) );
	}
}

void raveloxmidi_alsa_teardown( void )
{
	int i = 0;

	logging_printf(LOGGING_DEBUG,"raveloxmidi_alsa_teardown: start\n");
	if( num_inputs > 0 )
	{
		pthread_mutex_lock( &input_table_lock );
		for( i = num_inputs - 1 ; i >= 0; i-- )
		{
			raveloxmidi_alsa_handle_destroy( inputs[i] );
			inputs[i] = NULL;
			num_outputs--;
		}
		num_inputs = 0;
		free( inputs );
		inputs = NULL;
		pthread_mutex_unlock( &input_table_lock );
	}

	if( num_outputs > 0 )
	{
		pthread_mutex_lock( &output_table_lock );
		for( i = num_outputs - 1 ; i >= 0; i-- )
		{
			raveloxmidi_alsa_handle_destroy( outputs[i] );
			outputs[i] = NULL;
			num_outputs--;
		}
		num_outputs = 0;
		free( outputs );
		outputs = NULL;
		pthread_mutex_unlock( &output_table_lock );
	}

	pthread_mutex_lock( &poll_table_lock );
	FREENULL( "poll_descriptors", (void **)&poll_descriptors );
	pthread_mutex_unlock( &poll_table_lock );

	pthread_mutex_destroy( &output_table_lock );
	pthread_mutex_destroy( &input_table_lock );
	pthread_mutex_destroy( &poll_table_lock );

	logging_printf(LOGGING_DEBUG,"raveloxmidi_alsa_teardown: end\n");
}

void raveloxmidi_alsa_dump_rawmidi( snd_rawmidi_t *rawmidi )
{
	snd_rawmidi_info_t *info = NULL;
	snd_rawmidi_params_t *params = NULL;
	int card_number = 0;
	unsigned int device_number = 0, rawmidi_flags = 0;
	const char *hw_id = NULL, *hw_driver_name = NULL, *handle_id = NULL, *sub_name = NULL;
	size_t available_min = 0, buffer_size = 0;
	int active_sensing = 0;

	DEBUG_ONLY;
	if( ! rawmidi ) return;

	snd_rawmidi_info_malloc( &info );
	snd_rawmidi_info( rawmidi, info );
	card_number = snd_rawmidi_info_get_card( info );
	device_number = snd_rawmidi_info_get_device( info );
	rawmidi_flags = snd_rawmidi_info_get_flags( info );
	hw_id = snd_rawmidi_info_get_id( info );
	hw_driver_name = snd_rawmidi_info_get_name( info );
	sub_name = snd_rawmidi_info_get_subdevice_name( info );
	handle_id = snd_rawmidi_name( rawmidi );
	logging_printf(LOGGING_DEBUG, "rawmidi: handle=\"%s\" hw_id=\"%s\" hw_driver_name=\"%s\" subdevice_name=\"%s\" flags=%u card=%d device=%u\n",
		handle_id, hw_id, hw_driver_name, sub_name, rawmidi_flags, card_number, device_number );
	snd_rawmidi_info_free( info );

	snd_rawmidi_params_malloc( &params );
	snd_rawmidi_params_current( rawmidi, params );
	available_min = snd_rawmidi_params_get_avail_min( params );
	buffer_size = snd_rawmidi_params_get_buffer_size( params );
	active_sensing = snd_rawmidi_params_get_no_active_sensing( params );
	logging_printf(LOGGING_DEBUG, "\tavailable_min=%lu, buffer_size=%lu, active_sensing=%d\n", available_min, buffer_size, active_sensing);
	snd_rawmidi_params_free( params );
}

int raveloxmidi_alsa_out_available( void )
{
	logging_printf( LOGGING_DEBUG, "raveloxmidi_alsa_out_available: %d\n", num_outputs > 0 );
	return num_outputs > 0;
}

int raveloxmidi_alsa_in_available( void )
{
	logging_printf( LOGGING_DEBUG, "raveloxmidi_alsa_in_available: %d\n", num_inputs > 0 );
	return num_inputs > 0;
}

int raveloxmidi_alsa_write( unsigned char *buffer, size_t buffer_size )
{
	int i = 0;
	size_t bytes_written = 0;

	for( i = 0; i < num_outputs; i++ )
	{
		bytes_written = snd_rawmidi_write( outputs[i], buffer, buffer_size );
		logging_printf(LOGGING_DEBUG,"raveloxmidi_alsa_write: handle index=%d bytes_written=%u\n", i, bytes_written );
	}

	return bytes_written;
}

int raveloxmidi_alsa_read( snd_rawmidi_t *handle, unsigned char *buffer, size_t buffer_size )
{
	ssize_t bytes_read = -1;
	
	if( ! buffer )
	{
		logging_printf(LOGGING_DEBUG,"raveloxmidi_alsa_read: No buffer provided\n");
		return -1;
	}

	if( buffer_size <= 0 )
	{
		logging_printf(LOGGING_DEBUG,"raveloxmidi_alsa_read: Buffer size is <= 0\n");
 		return -1;
	}

	if( handle )
	{
		bytes_read = snd_rawmidi_read( handle, buffer, buffer_size );
	} else {
		logging_printf(LOGGING_DEBUG,"raveloxmidi_alsa_read: No input handle\n");
	}

	return bytes_read;
}

int raveloxmidi_alsa_poll( int timeout )
{
	int err = 0;
	int ret = 0;
	unsigned short int revents = 0;
	int i = 0;
	int shutdown_fd = 0;

	if( !poll_descriptors ) return 0;
	if( ! raveloxmidi_alsa_in_available() ) return 0; 
 
	shutdown_fd = net_socket_get_shutdown_fd();

	err = poll( poll_descriptors, num_poll_descriptors, timeout );

	if( (errno != 0) && (errno != EAGAIN) )
	{
		logging_printf( LOGGING_WARN, "raveloxmidi_alsa_poll: %s\n", strerror( errno ) );
	} 

	if( err > 0 )
	{
		pthread_mutex_lock( &poll_table_lock );
		for( i = 0 ; i < num_poll_descriptors; i++ )
		{
			if( poll_descriptors[i].fd == shutdown_fd )
			{
				if( poll_descriptors[i].revents & POLLIN )
				{
					ret = 2;
					break;
				}
			} else {
				if( poll_descriptors[i].revents & POLLIN )
				{
					ret = 1;
					break;
				}
			}
		}
		pthread_mutex_unlock( &poll_table_lock );
	}

	return ret;
}

static void raveloxmidi_alsa_add_poll_fd( snd_rawmidi_t *handle, int fd )
{
	struct pollfd *new_fds = NULL;
	raveloxmidi_socket_t *socket = NULL;

	if( fd < 0 ) return;
	
	new_fds = (struct pollfd * )realloc( poll_descriptors, ( num_poll_descriptors + 1 ) * sizeof( struct pollfd ) );
	if( ! new_fds )
	{
		logging_printf( LOGGING_ERROR, "raveloxmidi_alsa_add_poll_fd: Insufficient memory to extend poll fd table\n");
		return;
	}

	pthread_mutex_lock( &poll_table_lock );

	poll_descriptors = new_fds;
	poll_descriptors[ num_poll_descriptors ].fd = fd;
	poll_descriptors[ num_poll_descriptors ].events = POLLIN | POLLERR | POLLNVAL | POLLHUP;
	poll_descriptors[ num_poll_descriptors ].revents = 0;
	num_poll_descriptors++;

	pthread_mutex_unlock( &poll_table_lock );

	/* Add the file descriptor to the socket list */
	socket = net_socket_add( fd );
	if( socket )
	{
		socket->type = RAVELOXMIDI_SOCKET_ALSA_TYPE;
		socket->handle = handle;
	}

}

void raveloxmidi_alsa_set_poll_fds( snd_rawmidi_t *handle )
{
	struct pollfd *current_fds = NULL;
	int num_current_fds = 0;
	int i = 0;

	if( ! handle ) return;

	num_current_fds = snd_rawmidi_poll_descriptors_count( handle );

	if( num_current_fds <= 0 ) return;

	current_fds = (struct pollfd *)malloc( (num_current_fds+1) * sizeof( struct pollfd ) );

	if(! current_fds )
	{
		logging_printf( LOGGING_ERROR, "raveloxmidi_alsa_set_poll_fds: Insufficient memory to store current fds\n");
		return;
	}

	memset( current_fds, 0, (num_current_fds+1) * sizeof( struct pollfd ) );

	if(snd_rawmidi_poll_descriptors( handle, current_fds, num_current_fds ) == num_current_fds )
	{
		for( i = 0 ; i < num_current_fds; i++ )
		{
			logging_printf( LOGGING_DEBUG, "raveloxmidi_alsa_set_poll_fds: current_fd[%u]=%u\n", i, current_fds[i].fd);
			raveloxmidi_alsa_add_poll_fd( handle, current_fds[i].fd );
		}
	}

	free( current_fds );

}

static void * raveloxmidi_alsa_listener( void *data )
{
	long socket_timeout = 0;
	int poll_result = 0;
	int i = 0;

	logging_printf(LOGGING_DEBUG, "raveloxmidi_alsa_listener: Thread started\n");
	raveloxmidi_alsa_add_poll_fd( NULL, net_socket_get_shutdown_fd() );

	socket_timeout = config_long_get("socket.timeout");

	if( socket_timeout <= 0 )
	{
		socket_timeout = 30;
	}

	// Set socket timeout to be milliseconds
	socket_timeout *= 1000;

	do {
		poll_result = raveloxmidi_alsa_poll( socket_timeout );

		logging_printf( LOGGING_DEBUG, "raveloxmidi_alsa_listener: poll_result = %d\n", poll_result );

		if( poll_result == 1 )
		{
			pthread_mutex_lock( &poll_table_lock );
			for( i = 0; i < num_poll_descriptors; i++ )
			{
				if( poll_descriptors[i].revents & POLLIN )
				{
					net_socket_read( poll_descriptors[i].fd );
				}
			}
			pthread_mutex_unlock( &poll_table_lock );
		}

		if( poll_result == 2 )
		{
			logging_printf(LOGGING_DEBUG, "raveloxmidi_alsa_listener: Data received on shutdown pipe\n");
		}
	} while ( net_socket_get_shutdown_status() == OK );

	raveloxmidi_alsa_teardown();

	logging_printf(LOGGING_DEBUG, "raveloxmidi_alsa_listener: Thread stopped\n");

	return NULL;
}

int raveloxmidi_alsa_loop()
{
	// Only start the thread if an input handle is available
	if( raveloxmidi_alsa_in_available() )
	{
		pthread_create( &alsa_listener_thread, NULL, raveloxmidi_alsa_listener, NULL );
	}
	return 0;
}

void raveloxmidi_wait_for_alsa(void)
{
	if( raveloxmidi_alsa_in_available() )
	{
		pthread_join( alsa_listener_thread, NULL );
		logging_printf( LOGGING_DEBUG, "raveloxmidi_wait_for_alsa: %s\n", strerror(errno) );
	}
}
#endif
