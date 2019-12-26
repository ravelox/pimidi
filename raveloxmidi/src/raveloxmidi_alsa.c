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

#include "raveloxmidi_alsa.h"
#include "utils.h"
#include "logging.h"

static snd_rawmidi_t *input_handle = NULL;
static snd_rawmidi_t *output_handle = NULL;

static int pd_count = 0;
static struct pollfd *poll_descriptors = NULL;

extern int errno;


void raveloxmidi_alsa_init( char *input_name , char *output_name , size_t buffer_size)
{
	int ret = 0;

	if( input_name )
	{
		ret = snd_rawmidi_open( &input_handle, NULL, input_name, SND_RAWMIDI_NONBLOCK );
		logging_printf(LOGGING_DEBUG,"raveloxmidi_alsa_init input: device=%s ret=%d %s\n", input_name, ret, snd_strerror( ret ) );
		
		if( ret == 0 )
		{
			if( (buffer_size > RAVELOXMIDI_ALSA_DEFAULT_BUFFER) && (buffer_size <= RAVELOXMIDI_ALSA_MAX_BUFFER) )
			{
				snd_rawmidi_params_t *params;
				snd_rawmidi_params_malloc( &params );
				snd_rawmidi_params_current( input_handle, params );
				snd_rawmidi_params_set_buffer_size( input_handle, params, buffer_size );
				snd_rawmidi_params( input_handle, params );
				snd_rawmidi_params_free( params );
			}

			raveloxmidi_alsa_dump_rawmidi( input_handle );

		}
	}

	if( output_name )
	{
		ret = snd_rawmidi_open( NULL, &output_handle, output_name, SND_RAWMIDI_NONBLOCK );
		logging_printf(LOGGING_DEBUG,"raveloxmidi_alsa_init output: device=%s ret=%d %s\n", output_name, ret, snd_strerror( ret ) );
		if( ret == 0 ) 
		{
			raveloxmidi_alsa_dump_rawmidi( output_handle );
		}
	}
}

void raveloxmidi_alsa_handle_destroy( snd_rawmidi_t *rawmidi )
{
	if( ! rawmidi ) return;

	snd_rawmidi_drain( rawmidi );
	snd_rawmidi_close( rawmidi );
}

void raveloxmidi_alsa_teardown( void )
{
	raveloxmidi_alsa_handle_destroy( input_handle );
	raveloxmidi_alsa_handle_destroy( output_handle );

	FREENULL( "poll_descriptors", (void **)&poll_descriptors );

	input_handle = NULL;
	output_handle = NULL;
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
	return output_handle != NULL;
}

int raveloxmidi_alsa_in_available( void )
{
	return input_handle != NULL;
}

int raveloxmidi_alsa_write( unsigned char *buffer, size_t buffer_size )
{
	size_t bytes_written = 0;

	if( output_handle ) 
	{
		bytes_written = snd_rawmidi_write( output_handle, buffer, buffer_size );
		logging_printf(LOGGING_DEBUG,"raveloxmidi_alsa_write: bytes_written=%u\n", bytes_written );
	}

	return bytes_written;
}

int raveloxmidi_alsa_read( unsigned char *buffer, size_t buffer_size )
{
	ssize_t bytes_read = -1;
	
	if( ! buffer )
	{
		return -1;
	}
	
	if( buffer_size == 0 )
	{
		return -1;
	}

	if( input_handle )
	{
		bytes_read = snd_rawmidi_read( input_handle, buffer, buffer_size );
	}

	return bytes_read;
}


int raveloxmidi_alsa_poll( int timeout )
{
	int err = 0;
	int ret = 0;
	unsigned short int revents = 0;

	if( !poll_descriptors ) return 0;
	if( ! raveloxmidi_alsa_in_available() ) return 0; 
 
	err = poll( poll_descriptors, pd_count + 1, timeout );

	if( (errno != 0) && (errno != EAGAIN) )
	{
		logging_printf( LOGGING_WARN, "raveloxmidi_alsa_poll: %s\n", strerror( errno ) );
	} 

	if( err > 0 )
	{
		err = snd_rawmidi_poll_descriptors_revents( input_handle, poll_descriptors, pd_count , &revents );
		if( revents & POLLIN ) ret = 1;
	}

	return ret;
}

void raveloxmidi_alsa_set_poll_fds( int fd )
{
	int i = 0;

	if( ! input_handle ) return;

	pd_count = snd_rawmidi_poll_descriptors_count( input_handle );
	poll_descriptors = (struct pollfd *)malloc( (pd_count+1) * sizeof( struct pollfd ) );
	memset( poll_descriptors, 0, (pd_count+1) * sizeof( struct pollfd ) );
	if(snd_rawmidi_poll_descriptors( input_handle, poll_descriptors, pd_count ) == pd_count )
	{
		for( i = 0 ; i < pd_count; i++ )
		{
			logging_printf( LOGGING_DEBUG, "raveloxmidi_alsa_init: poll_descriptor[%u]=%u\n", i, poll_descriptors[i].fd);
			poll_descriptors[i].events = POLLIN | POLLERR | POLLNVAL | POLLHUP;
		}
	}

	poll_descriptors[pd_count].fd = fd;
	poll_descriptors[pd_count].events = POLLIN | POLLERR | POLLNVAL | POLLHUP;
}

#endif
