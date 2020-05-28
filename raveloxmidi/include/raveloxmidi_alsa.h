/*
   This file is part of raveloxmidi.

   Copyright (C) 2020 Dave Kelly

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

#ifdef HAVE_ALSA

#ifndef _RAVELOXMIDI_ALSA_H
#define _RAVELOXMIDI_ALSA_H

#include <alsa/asoundlib.h>

void raveloxmidi_alsa_list_rawmidi_devices( void );
void raveloxmidi_alsa_init( char *input_name, char *output_name , size_t buffer_size);
void raveloxmidi_alsa_handle_destroy( void **rawmidi );
void raveloxmidi_alsa_teardown( void );

int raveloxmidi_alsa_card_number( snd_rawmidi_t *rawmidi );
int raveloxmidi_alsa_device_number( snd_rawmidi_t *rawmidi );
void raveloxmidi_alsa_dump_rawmidi( void *data );

int raveloxmidi_alsa_out_available( void );
int raveloxmidi_alsa_in_available( void );

int raveloxmidi_alsa_write( unsigned char *buffer, size_t buffer_size, int originator_card);
int raveloxmidi_alsa_read( snd_rawmidi_t *handle, unsigned char *buffer, size_t read_size); 
int raveloxmidi_alsa_poll( int timeout );
void raveloxmidi_alsa_set_poll_fds( snd_rawmidi_t *handle );

int raveloxmidi_alsa_loop( void );
void raveloxmidi_wait_for_alsa( void );

int raveloxmidi_alsa_device_hash( snd_rawmidi_t *rawmidi );

#define RAVELOXMIDI_ALSA_INPUT	-2
#define RAVELOXMIDI_ALSA_DEFAULT_BUFFER	4096
#define RAVELOXMIDI_ALSA_MAX_BUFFER	1048576

#endif

#endif
