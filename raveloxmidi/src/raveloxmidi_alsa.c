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
#include <string.h>

#include <asoundlib.h>

#include <signal.h>
#include <pthread.h>

#include "net_socket.h"
#include "raveloxmidi_alsa.h"
#include "data_table.h"
#include "utils.h"
#include "logging.h"
#include "raveloxmidi_config.h"

/* Table of ALSA output handles */
static data_table_t *outputs = NULL;

/* Table of ALSA input handles */
static data_table_t *inputs = NULL;

/* Table of input file descriptors for polling */
static int num_poll_descriptors = 0;
static struct pollfd *poll_descriptors = NULL;
static pthread_mutex_t poll_descriptors_table_lock;

static pthread_t alsa_listener_thread;

extern int errno;

static void poll_descriptors_lock( void )
{
	pthread_mutex_lock( &poll_descriptors_table_lock );
}

static void poll_descriptors_unlock( void )
{
	pthread_mutex_unlock( &poll_descriptors_table_lock );
}

/* Open an output handle for the specified ALSA device and add it to the list */
static void raveloxmidi_alsa_add_output( const char *device_name )
{
	snd_rawmidi_t *output_handle = NULL;
	int ret = 0;

	if( ! device_name ) return;

	ret = snd_rawmidi_open( NULL, &output_handle, device_name, SND_RAWMIDI_NONBLOCK );
	logging_printf(LOGGING_DEBUG,"raveloxmidi_alsa_add_output: device=%s ret=%d %s\n", device_name, ret, snd_strerror( ret ) );
	if( ret != 0 ) return;

	raveloxmidi_alsa_dump_rawmidi( output_handle );

	if( data_table_add_item( outputs, output_handle ) == 0 )
	{
		/* Close the open handle */
		raveloxmidi_alsa_handle_destroy( (void **)&output_handle );
		logging_printf( LOGGING_ERROR, "raveloxmidi_alsa_add_output: Insufficient memory for new item: %s\n", device_name);
		return;
	}
}

/* Open an input handle for the specified ALSA device and add it to the list */
static void raveloxmidi_alsa_add_input( const char *device_name , size_t buffer_size)
{
	snd_rawmidi_t *input_handle = NULL;
	int ret = 0;

	if( ! device_name ) return;

	ret = snd_rawmidi_open( &input_handle, NULL, device_name, SND_RAWMIDI_NONBLOCK );
	logging_printf(LOGGING_DEBUG,"raveloxmidi_alsa_add_input: device=%s ret=%d %s\n", device_name, ret, snd_strerror( ret ) );

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

	if(data_table_add_item( inputs, input_handle ) == 0 )
	{ 
		/* Close the open handle */
		raveloxmidi_alsa_handle_destroy( (void **)&input_handle );
		logging_printf( LOGGING_ERROR, "raveloxmidi_alsa_add_input: Insufficient memory for new item: %s\n", device_name);
		return;
	}

	/* Add the file descriptors to the polling table */
	raveloxmidi_alsa_set_poll_fds( input_handle );
}

void raveloxmidi_alsa_init( char *input_name , char *output_name , size_t buffer_size)
{
	outputs = data_table_create( "ALSA outputs", raveloxmidi_alsa_handle_destroy , raveloxmidi_alsa_dump_rawmidi);
	if(! outputs )
	{
		logging_printf(LOGGING_ERROR, "raveloxmidi_alsa_init: Unable to create outputs table\n");
		return;
	}

	inputs = data_table_create( "ALSA inputs", raveloxmidi_alsa_handle_destroy , raveloxmidi_alsa_dump_rawmidi);
	if(! inputs )
	{
		logging_printf(LOGGING_ERROR, "raveloxmidi_alsa_init: Unable to create inputs table\n");
		return;
	}

	pthread_mutex_init( &poll_descriptors_table_lock , NULL);

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

void raveloxmidi_alsa_handle_destroy( void **data )
{
	int status = 0;
	snd_rawmidi_t *rawmidi = NULL;

	if(! data ) return;

	logging_printf( LOGGING_DEBUG, "raveloxmidi_alsa_handle_destroy\n");

	rawmidi = ( snd_rawmidi_t *)(*data);
	if( ! rawmidi ) return;

	status = snd_rawmidi_drain( rawmidi );
	if( status > 0 )
	{
		logging_printf(LOGGING_ERROR, "raveloxmidi_alsa_handle_destroy: snd_rawmidi_drain()=%d : %s\n", status, snd_strerror(status) );
	}

	status = snd_rawmidi_close( rawmidi );
	if( status > 0 )
	{
		logging_printf(LOGGING_WARN, "raveloxmidi_alsa_handle_destroy: snd_rawmidi_close()=%d : %s\n", status, snd_strerror(status) );
	}
}

void raveloxmidi_alsa_teardown( void )
{
	logging_printf(LOGGING_DEBUG,"raveloxmidi_alsa_teardown: start\n");

	data_table_destroy( &inputs );
	data_table_destroy( &outputs );

	poll_descriptors_lock();
	FREENULL( "raveloxmidi_alsa_teardown:poll_descriptors", (void **)&poll_descriptors );
	poll_descriptors_unlock();

	pthread_mutex_destroy( &poll_descriptors_table_lock );

	snd_config_update_free_global();

	logging_printf(LOGGING_DEBUG,"raveloxmidi_alsa_teardown: end\n");
}

int raveloxmidi_alsa_card_number( snd_rawmidi_t *rawmidi )
{
	int card_number = -1;

	if( rawmidi )
	{
		snd_rawmidi_info_t *info = NULL;

		snd_rawmidi_info_malloc( &info );
		snd_rawmidi_info( rawmidi, info );
		card_number = snd_rawmidi_info_get_card( info );
		snd_rawmidi_info_free( info );
	}

	return card_number;
}

void raveloxmidi_alsa_dump_rawmidi( void *data )
{
	snd_rawmidi_t *rawmidi = NULL;
	snd_rawmidi_info_t *info = NULL;
	snd_rawmidi_params_t *params = NULL;
	int card_number = 0;
	unsigned int device_number = 0, rawmidi_flags = 0;
	const char *name = NULL, *hw_id = NULL, *hw_driver_name = NULL, *handle_id = NULL, *sub_name = NULL;
	size_t available_min = 0, buffer_size = 0;
	int active_sensing = 0;

	DEBUG_ONLY;

	if( ! data ) return;

	rawmidi = (snd_rawmidi_t *)data;
	if( ! rawmidi ) return;

	snd_rawmidi_info_malloc( &info );
	snd_rawmidi_info( rawmidi, info );
	name = snd_rawmidi_info_get_name( info );
	card_number = snd_rawmidi_info_get_card( info );
	device_number = snd_rawmidi_info_get_device( info );
	rawmidi_flags = snd_rawmidi_info_get_flags( info );
	hw_id = snd_rawmidi_info_get_id( info );
	hw_driver_name = snd_rawmidi_info_get_name( info );
	sub_name = snd_rawmidi_info_get_subdevice_name( info );
	handle_id = snd_rawmidi_name( rawmidi );
	logging_printf(LOGGING_DEBUG, "rawmidi: name=[%s] handle=[%s] hw_id=[%s] hw_driver_name=[%s] subdevice_name=[%s] flags=%u card=%d device=%u\n",
		name, handle_id, hw_id, hw_driver_name, sub_name, rawmidi_flags, card_number, device_number );
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
	size_t unused = 0;
	size_t count = 0;

	unused = data_table_unused_count( outputs );
	count = data_table_item_count( outputs );

	return ( count > unused );
}

int raveloxmidi_alsa_in_available( void )
{
	size_t unused = 0;
	size_t count = 0;

	unused = data_table_unused_count( inputs );
	count = data_table_item_count( inputs );

	return ( count > unused );
}

int raveloxmidi_alsa_write( unsigned char *buffer, size_t buffer_size, int originator_card )
{
	int i = 0;
	size_t num_outputs = 0;
	size_t bytes_written = 0;

	num_outputs = data_table_item_count( outputs );
	for( i = 0; i < num_outputs; i++ )
	{
		if( data_table_item_is_unused( outputs, i ) == 0 )
		{
			snd_rawmidi_t *handle = NULL;
			handle = (snd_rawmidi_t *)data_table_item_get( outputs, i );
			if( handle )
			{
				int card_number = 0;
				int writeback = 0;

				card_number = raveloxmidi_alsa_card_number( handle );

				// Only write out if this is not the same card that provided the data
				writeback = is_yes( config_string_get("alsa.writeback") );

				if( ( originator_card != card_number ) || ( writeback == 1 ) )
				{
					bytes_written = snd_rawmidi_write( handle, buffer, buffer_size );
					logging_printf(LOGGING_DEBUG,"raveloxmidi_alsa_write: handle index=%d originator=%u card=%u bytes_written=%u\n",
						i, originator_card, card_number, bytes_written );
				} else {
					logging_printf( LOGGING_DEBUG,"raveloxmidi_alsa_write: not writing to the same card\n");
				}
			}
		}
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
		poll_descriptors_lock();
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
		poll_descriptors_unlock();
	}

	return ret;
}

static void raveloxmidi_alsa_add_poll_fd( snd_rawmidi_t *handle, int fd )
{
	struct pollfd *new_fds = NULL;
	raveloxmidi_socket_t *socket = NULL;

	if( fd < 0 ) return;
	
	poll_descriptors_lock();

	new_fds = (struct pollfd * )realloc( poll_descriptors, ( num_poll_descriptors + 1 ) * sizeof( struct pollfd ) );
	if( ! new_fds )
	{
		logging_printf( LOGGING_ERROR, "raveloxmidi_alsa_add_poll_fd: Insufficient memory to extend poll fd table\n");
		goto add_poll_end;
	}


	poll_descriptors = new_fds;
	poll_descriptors[ num_poll_descriptors ].fd = fd;
	poll_descriptors[ num_poll_descriptors ].events = POLLIN | POLLERR | POLLNVAL | POLLHUP;
	poll_descriptors[ num_poll_descriptors ].revents = 0;
	num_poll_descriptors++;

	/* Add the file descriptor to the socket list */
	socket = net_socket_add( fd );
	if( socket )
	{
		socket->type = RAVELOXMIDI_SOCKET_ALSA_TYPE;
		socket->handle = handle;
		socket->card_number = raveloxmidi_alsa_card_number( handle );
	}
add_poll_end:
	poll_descriptors_unlock();
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

		if( poll_result > 0 )
		{
			logging_printf( LOGGING_DEBUG, "raveloxmidi_alsa_listener: poll_result = %d\n", poll_result );
		}

		if( poll_result == 1 )
		{
			poll_descriptors_lock();
			for( i = 0; i < num_poll_descriptors; i++ )
			{
				if( poll_descriptors[i].revents & POLLIN )
				{
					net_socket_read( poll_descriptors[i].fd );
				}
			}
			poll_descriptors_unlock();
		}

		if( poll_result == 2 )
		{
			logging_printf(LOGGING_DEBUG, "raveloxmidi_alsa_listener: Data received on shutdown pipe\n");
		}
	} while ( net_socket_get_shutdown_status() == OK );

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
