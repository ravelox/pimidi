/*
   This file is part of raveloxmidi.

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
#include <ctype.h>

#include "config.h"

#include "midi_command.h"
#include "midi_payload.h"
#include "utils.h"

#include "logging.h"

// Sec 3.2 of RFC6295
// As we note above, the first channel command in the MIDI list MUST
// include a status octet.  However, the corresponding command in the
//original MIDI source data stream might not have a status octet (in
// this case, the source would be coding the command using running
// status)
typedef struct midi_running_status_t {
	int fd;
	unsigned char status;
} midi_running_status_t;
static midi_running_status_t *running_status;
static int running_status_size;

unsigned char *midi_payload_get_running_status( int fd )
{
	int i;

	if (!running_status) 
	{
		running_status = (midi_running_status_t *)malloc(sizeof(midi_running_status_t));

		if (!running_status)
		{
			logging_printf(LOGGING_ERROR, "midi_payload_get_running_status: Unable to allocate memory for running status\n");
			return NULL;
		}

		running_status_size = 1;

		running_status[0].fd = fd;
		running_status[0].status = 0;

		return &(running_status[0].status);
	}

	for (i = 0; i < running_status_size; i++)
	{
		if (running_status[i].fd == fd)
			return &(running_status[i].status);
	}

	running_status = (midi_running_status_t *)realloc(running_status, (running_status_size + 1) * sizeof(midi_running_status_t));

	if (!running_status)
	{
		logging_printf(LOGGING_ERROR, "midi_payload_get_running_status: Unable to reallocate memory for running status\n");
		return NULL;
	}

	running_status_size += 1;

	running_status[i].fd = fd;
	running_status[i].status = 0;

	return &(running_status[i].status);
}

void midi_payload_set_running_status( int fd, unsigned char status )
{
	if (status >= 0xf8)
		return;

	unsigned char *running_status = midi_payload_get_running_status(fd);

	if (running_status) 
		*running_status = status < 0xf0 ? status : 0;
}

void midi_payload_destroy( midi_payload_t **payload )
{
	if( ! payload ) return;
	if( ! *payload ) return;

	if( (*payload)->buffer )
	{
		FREENULL( "midi_payload:payload->buffer",(void **)&((*payload)->buffer) );
	}

	if( (*payload)->header )
	{
		FREENULL( "midi_payload:payload->header",(void **)&((*payload)->header) );
	}

	FREENULL( "midi_payload",(void **)payload );
}

void midi_payload_reset( midi_payload_t *payload )
{
	if( ! payload ) return;

	if( payload->buffer ) free( payload->buffer );
	payload->buffer = NULL;

	if( payload->header )
	{
		payload->header->B = 0;
		payload->header->J = 0;
		payload->header->Z = 0;
		payload->header->P = 0;
		payload->header->len = 0;
	}
}

midi_payload_t * midi_payload_create( void )
{
	midi_payload_t *payload = NULL;

	payload = ( midi_payload_t * )malloc( sizeof( midi_payload_t ) );

	if( ! payload ) return NULL;

	memset( payload, 0, sizeof(midi_payload_t ) );

	payload->header = ( midi_payload_header_t *)malloc( sizeof( midi_payload_header_t ) );
	if( ! payload->header )
	{
		logging_printf( LOGGING_ERROR, "midi_payload_create: Not enough memory\n");
		free( payload );
		return NULL;
	}
	midi_payload_reset( payload );

	return payload;
}

void midi_payload_set_b( midi_payload_t *payload )
{
	if( ! payload) return;
	if( ! payload->header) return;
	payload->header->B = 1;
}

void midi_payload_set_j( midi_payload_t *payload )
{
	if( ! payload) return;
	if( ! payload->header) return;
	payload->header->J = 1;
}

void midi_payload_set_z( midi_payload_t *payload )
{
	if( ! payload) return;
	if( ! payload->header) return;
	payload->header->Z = 1;
}

void midi_payload_set_p( midi_payload_t *payload )
{
	if( ! payload) return;
	if( ! payload->header) return;
	payload->header->P = 1;
}

void midi_payload_unset_b( midi_payload_t *payload )
{
	if( ! payload) return;
	if( ! payload->header) return;
	payload->header->B = 0;
}

void midi_payload_unset_j( midi_payload_t *payload )
{
	if( ! payload) return;
	if( ! payload->header) return;
	payload->header->J = 0;
}

void midi_payload_unset_z( midi_payload_t *payload )
{
	if( ! payload) return;
	if( ! payload->header) return;
	payload->header->Z = 0;
}

void midi_payload_unset_p( midi_payload_t *payload )
{
	if( ! payload) return;
	if( ! payload->header) return;
	payload->header->P = 0;
}


void midi_payload_set_buffer( midi_payload_t *payload, unsigned char *buffer , size_t *buffer_size, int fd )
{
	int status_present = 0;
	if( ! payload ) return;

	logging_printf( LOGGING_DEBUG, "midi_payload_set_buffer: payload=%p,buffer=%p,buffer_size=%u\n", payload, buffer, *buffer_size);

	status_present = buffer[0] & 0x80;

	/* If the first byte doesn't include a status then add one */
	if( ! status_present )
	{
		*buffer_size += 1;
	}

	payload->header->len = *buffer_size;
	payload->buffer = (unsigned char *)malloc( *buffer_size );
	if( ! payload->buffer )
	{
		payload->header->len = 0;
	} else {
		if( status_present )
		{
			memcpy( payload->buffer, buffer, *buffer_size );
			midi_payload_set_running_status(fd, buffer[0]);
		} else {
			memcpy( payload->buffer + 1, buffer, *buffer_size - 1 );
			unsigned char *running_status = midi_payload_get_running_status(fd);
			payload->buffer[0] = running_status ? *running_status : 0;
		}
	}
}


void midi_payload_header_dump( midi_payload_header_t *header )
{
	DEBUG_ONLY;
	if( ! header ) return;

	logging_printf( LOGGING_DEBUG, "MIDI Payload(B=%d,J=%d,Z=%d,P=%d,payload_length=%u)\n",
		header->B, header->J,header->Z, header->P, header->len);
}

void midi_payload_pack( midi_payload_t *payload, unsigned char **buffer, size_t *buffer_size)
{
	uint8_t temp_header = 0;
	unsigned char *p = NULL;

	*buffer = NULL;
	*buffer_size = 0;

	if( ! payload ) return;

	if( ! payload->buffer ) return;
	if( ! payload->header ) return;

	midi_payload_header_dump( payload->header );

	*buffer_size = 1 + payload->header->len + (payload->header->len > 15 ? 1 : 0);
	*buffer = (unsigned char *)malloc( *buffer_size );

	if( ! *buffer )
	{
		logging_printf(LOGGING_ERROR, "midi_payload_pack: Insufficient memory\n");
		return;
	}

	p = *buffer;

	if( payload->header->B) temp_header |= PAYLOAD_HEADER_B;
	if( payload->header->J) temp_header |= PAYLOAD_HEADER_J;
	if( payload->header->Z) temp_header |= PAYLOAD_HEADER_Z;
	if( payload->header->P) temp_header |= PAYLOAD_HEADER_P;

	*p = temp_header;

	if( payload->header->len <= 15 )
	{
		*p |= ( payload->header->len & 0x0f );
		p++;
	} else {
		temp_header |= (payload->header->len & 0x0f00 ) >> 8;
		*p = temp_header;
		p++;
		*p =  (payload->header->len & 0x00ff);
		p++;
	}

	memcpy( p, payload->buffer, payload->header->len );

	logging_printf(LOGGING_DEBUG, "midi_payload_pack: buffer=%p,len=%u\n", payload->buffer, payload->header->len );
}

void midi_payload_unpack( midi_payload_t **payload, unsigned char *buffer, size_t buffer_len )
{
	unsigned char *p;
	uint16_t temp_len;
	size_t current_len;

	if( !buffer ) return;
	if( buffer_len == 0 ) return;

	*payload = midi_payload_create();
	if( ! *payload ) return;

	p = buffer;
	current_len = buffer_len;

	/* Get the flags */
	if( *p & PAYLOAD_HEADER_B ) midi_payload_set_b( *payload );
	if( *p & PAYLOAD_HEADER_J ) midi_payload_set_j( *payload );
	if( *p & PAYLOAD_HEADER_Z ) midi_payload_set_z( *payload );
	if( *p & PAYLOAD_HEADER_P ) midi_payload_set_p( *payload );

	/* Check that there's enough buffer if the B flag indicates the length field is 12 bits */
	if( (*payload)->header->B && ( current_len == 1 ) )
	{
		logging_printf(LOGGING_ERROR, "midi_payload_unpack: B flag set but insufficent buffer data\n" );
		goto midi_payload_unpack_error;
	} 

	temp_len = ( *p & 0x0f );
	current_len--;

	/* If the B flag is set, get the next octet */
	if( (*payload)->header->B )
	{
		p++;
		current_len--;
		temp_len <<= 8;
		temp_len += *p;
	}

	(*payload)->header->len = temp_len;
	p++;

	/* Check that there's enough buffer for the defined length */
	if( current_len < temp_len ) 
	{
		logging_printf(LOGGING_ERROR, "midi_payload_unpack: Insufficent buffer data : current_len=%zu temp_len=%u\n", current_len, temp_len );
		goto midi_payload_unpack_error;
	}

	(*payload)->buffer = (unsigned char *)malloc( temp_len );
	if( ! (*payload)->buffer ) goto midi_payload_unpack_error;

	memcpy( (*payload)->buffer, p, temp_len );
	midi_payload_header_dump( (*payload)->header );

	goto midi_payload_unpack_success;

midi_payload_unpack_error:
	midi_payload_destroy( payload );
	*payload = NULL;

midi_payload_unpack_success:
	return;
}

void midi_payload_to_commands( midi_payload_t *payload, midi_payload_data_t data_type, midi_command_t **commands, size_t *num_commands, int fd )
{
	unsigned char *p;
	size_t current_len;
	uint64_t current_delta;
	unsigned char data_byte;
	char *command_description;
	enum midi_message_type_t message_type;
	size_t index, sysex_len;
	unsigned char *sysex_start_byte = NULL;
	

	*commands = NULL;
	*num_commands = 0;

	if( ! payload ) return;

	if( ! payload->header ) return;
	if( ! payload->buffer ) return;

	p = payload->buffer;
	current_len = payload->header->len;

	logging_printf( LOGGING_DEBUG, "midi_payload_to_commands: payload->header->len=%zu\n", payload->header->len);
	hex_dump( p, current_len );
	
	do 
	{
		logging_printf( LOGGING_DEBUG, "midi_payload_to_commands: current_len=%zu\n", current_len );
		current_delta = 0;
		if( data_type == MIDI_PAYLOAD_RTP )
		{
			/* If the Z flag == 0 then no delta time is present for the first midi command */
			if( ( payload->header->Z == 0 ) && ( *num_commands == 0 ) )
			{ 
				/*Do nothing*/
			} else {
				// Get the delta
				do
				{
					data_byte = *p;
					current_delta <<= 8;
					current_delta += ( data_byte & 0x7f );
					p++;
					current_len--;
				} while ( ( data_byte & 0x80 ) && current_len > 0 );
			}	
		}

		(*num_commands)++;

		logging_printf( LOGGING_DEBUG, "midi_payload_to_commands: next byte = 0x%02x\n", *p );
		midi_command_t *temp_commands = (midi_command_t * ) realloc( *commands, sizeof(midi_command_t) * (*num_commands) );

		if( ! temp_commands ) break;
		*commands = temp_commands;
		index = (*num_commands) - 1;

		(*commands)[index].delta = current_delta;
		(*commands)[index].data_len = 0;
		(*commands)[index].data = NULL;

		// Get the status byte. If bit 7 is not set, use the running status
		if( current_len > 0 )
		{
			data_byte = *p;
			unsigned char status = data_byte;

			if( status & 0x80 )
			{
				midi_payload_set_running_status(fd, status);
				p++;
				current_len--;
			}
			else
			{
				unsigned char *running_status = midi_payload_get_running_status(fd);
				status = running_status ? *running_status : 0;
			}

			(*commands)[index].status = status;
		}

		midi_command_map( &((*commands)[index]) , &command_description, &message_type );
		midi_command_dump( &((*commands)[index]));

		switch( message_type )
		{
			case MIDI_NOTE_OFF:
			case MIDI_NOTE_ON:
			case MIDI_POLY_PRESSURE:
			case MIDI_CONTROL_CHANGE:
			case MIDI_PITCH_BEND:
			case MIDI_SONG_POSITION:
				if( current_len >= 2 )
				{
					(*commands)[index].data = ( unsigned char * ) malloc( 2 );
					if( (*commands)[index].data )
					{
						memcpy( (*commands)[index].data, p, 2 );
						(*commands)[index].data_len = 2;
					}
					current_len -= 2;
					p+=2;
				}
				break;
			case MIDI_PROGRAM_CHANGE:
			case MIDI_CHANNEL_PRESSURE:
			case MIDI_TIME_CODE_QF:
			case MIDI_SONG_SELECT:
				if( current_len >= 1 )
				{
					(*commands)[index].data = ( unsigned char * ) malloc( 2 );
					memset( (*commands)[index].data, 0, 2 );
					if( (*commands)[index].data )
					{
						memcpy( (*commands)[index].data, p, 1);
						(*commands)[index].data_len = 1;
					}
					current_len -= 1;
					p+=1;
				}
				break;
			case MIDI_SYSEX:
			case MIDI_END_SYSEX:
				// Read until the end of the SYSEX block
				// RFC6295 sec 3.2 indicates that the following may occur
				// 0xF0-0xF0 == first block of multiple SysEx segments
				// 0xF7-0xF0 == middle block
				// 0xF7-0xF7 == end block
				// 0xF7-0xF4 == cancel ( FIXME: Need to code for this )
				sysex_start_byte = p;
				sysex_len = 0;
				do {
					data_byte = *p;
					sysex_len++;
					p++;
					current_len--;
				} while( ( current_len > 0 ) && ( (data_byte != 0xF7 )  && (data_byte != 0xF0) && (data_byte != 0xF4) ) );
				logging_printf( LOGGING_DEBUG, "SysEx end: 0x%02X\n", data_byte);
				if( ( message_type == MIDI_SYSEX ) && ( data_byte == 0xF0 ) )
				{
					logging_printf(LOGGING_DEBUG, "MIDI SYSEX first block\n");
				}
				if( ( message_type == MIDI_END_SYSEX ) && ( data_byte == 0xF0 ) )
				{
					logging_printf(LOGGING_DEBUG, "MIDI SYSEX middle block\n");
				}
				if( ( message_type == MIDI_END_SYSEX ) && ( data_byte == 0xF7 ) )
				{
					logging_printf(LOGGING_DEBUG, "MIDI SYSEX last block\n");
				}
				if( ( message_type == MIDI_END_SYSEX ) && ( data_byte == 0xF4 ) ) // FIXME: Need to code for this
				{
					logging_printf(LOGGING_DEBUG, "MIDI SYSEX cancel block\n");
				}
				if( sysex_len > 0 )
				{
					(*commands)[index].data = (unsigned char *) malloc( sysex_len );
					if( (*commands)[index].data ) 
					{
						memcpy( (*commands)[index].data, sysex_start_byte, sysex_len );
						(*commands)[index].data_len = sysex_len;
					}
				}
				break;
			case MIDI_TUNE_REQUEST:
			case MIDI_TIMING_CLOCK:
			case MIDI_START:
			case MIDI_CONTINUE:
			case MIDI_STOP:
			case MIDI_ACTIVE_SENSING:
			case MIDI_RESET:
			case MIDI_NULL:
				break;
		}
		
		switch( message_type )
		{
			case MIDI_NOTE_OFF:
			case MIDI_NOTE_ON:
			case MIDI_POLY_PRESSURE:
			case MIDI_CONTROL_CHANGE:
			case MIDI_PITCH_BEND:
			case MIDI_PROGRAM_CHANGE:
			case MIDI_CHANNEL_PRESSURE:
				logging_printf( LOGGING_INFO, "\tChannel: %u\n", (*commands)[index].status & 0x0f );
				break;
			case MIDI_SYSEX:
			case MIDI_TIME_CODE_QF:
			case MIDI_SONG_POSITION:
			case MIDI_SONG_SELECT:
			case MIDI_TUNE_REQUEST:
			case MIDI_END_SYSEX:
			case MIDI_TIMING_CLOCK:
			case MIDI_START:
			case MIDI_CONTINUE:
			case MIDI_STOP:
			case MIDI_ACTIVE_SENSING:
			case MIDI_RESET:
			case MIDI_NULL:
				break;
		}

	} while( current_len > 0 );
}

void midi_command_to_payload( midi_command_t *command, midi_payload_t **payload, int fd )
{
	size_t new_payload_size = 0;
	unsigned char *new_payload_buffer = NULL;

	if( ! command ) {
		*payload = NULL;
		return;
	}

	*payload = midi_payload_create();
	if( ! *payload )
	{
		logging_printf(LOGGING_ERROR, "midi_command_to_payload: Unable to allocate memory for new payload\n");
		return;
	}

	new_payload_size = command->data_len + 1;
	new_payload_buffer = (unsigned char *)malloc( new_payload_size );

	if( ! new_payload_buffer )
	{
		logging_printf(LOGGING_ERROR,"midi_command_to_payload: Unable to allocate memory for new payload buffer\n");
		midi_payload_destroy( &(*payload) );
		*payload = NULL;
	}

	new_payload_buffer[0] = command->status;
	memcpy( new_payload_buffer + 1 , command->data, command->data_len );

	midi_payload_set_buffer( *payload, new_payload_buffer, &new_payload_size, fd );

	free( new_payload_buffer );
}
