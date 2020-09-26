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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>

#include "midi_command.h"

#include "utils.h"

#include "raveloxmidi_config.h"
#include "logging.h"

#include "data_queue.h"

#include <errno.h>
extern int errno;

// File handle for storing inbound MIDI commands
extern int inbound_midi_fd;

#include "config.h"

#include "net_applemidi.h"
#include "net_response.h"
#include "net_socket.h"
#include "net_connection.h"

#include "applemidi_inv.h"
#include "applemidi_ok.h"
#include "applemidi_sync.h"
#include "applemidi_feedback.h"
#include "applemidi_by.h"

#include "midi_note.h"
#include "midi_control.h"
#include "midi_program.h"

#include "rtp_packet.h"
#include "midi_command.h"
#include "midi_payload.h"

#include "raveloxmidi_alsa.h"
#include "midi_sender.h"

#include "data_queue.h"
#include "data_context.h"

data_queue_t *midi_queue = NULL;
static unsigned int journal_enabled = 0;

void midi_sender_start( void )
{
	if( ! midi_queue )
	{
		logging_printf( LOGGING_ERROR, "midi_sender_start: No queue available\n");
	}

	data_queue_start( midi_queue );
}

void midi_sender_stop( void )
{
	if( ! midi_queue )
	{
		logging_printf( LOGGING_ERROR, "midi_sender_stop: No queue available\n");
	}

	data_queue_stop( midi_queue );
	data_queue_join( midi_queue );
	data_queue_destroy( &midi_queue );
}

void midi_sender_add( void *data, data_context_t *context )
{
	if( ! data ) return;
	data_context_acquire( context );
	data_queue_add( midi_queue, data, context );
}

void midi_sender_teardown( void )
{
}

/* Send MIDI commands to all connections */
void midi_sender_handler( void *data, void *context )
{
	data_context_t *data_context = NULL;
	midi_sender_context_t *sender_context = NULL;
	midi_command_t *command = NULL;

	uint32_t originator_ssrc = 0;
	int alsa_originator_card = 0;

	if( ! data ) return;
	command = (midi_command_t *)data;

	if( context )
	{
		data_context = (data_context_t *)context;
		if( data_context->data )
		{
			sender_context = (midi_sender_context_t *)(data_context->data);
			originator_ssrc = sender_context->ssrc;
			alsa_originator_card = sender_context->alsa_card_hash;
		}
		data_context_release( &data_context );
	}

	midi_sender_send_single( command, originator_ssrc, alsa_originator_card );

	midi_command_destroy( (void **)&command );
}

void midi_sender_send_single( midi_command_t *command, uint32_t originator_ssrc , int alsa_originator_card )
{
	midi_payload_t *single_midi_payload = NULL;
	unsigned char *packed_payload = NULL;
	size_t packed_payload_len = 0;
	enum midi_message_type_t message_type = 0;
	midi_note_t *midi_note = NULL;
	midi_control_t *midi_control = NULL;
	midi_program_t *midi_program = NULL;
	int i = 0;
	int total_connections = 0;
	char output_available = 0;
	unsigned char *raw_buffer = NULL;

	midi_command_to_payload( command, &single_midi_payload );
	if( ! single_midi_payload ) return;

	midi_command_map( command, NULL, &message_type );
	midi_command_dump( command );
	switch( message_type )
	{
		case MIDI_NOTE_OFF:
		case MIDI_NOTE_ON:
			midi_note_from_command( command, &midi_note);
			midi_note_dump( midi_note );
			break;
		case MIDI_CONTROL_CHANGE:	
			midi_control_from_command( command, &midi_control);
			midi_control_dump( midi_control );
			break;
		case MIDI_PROGRAM_CHANGE:
			midi_program_from_command( command, &midi_program);
			midi_program_dump( midi_program );
			break;
		default:
			break;
	}

	total_connections = net_ctx_get_num_connections();
	// Build the RTP packet for each connection
	for( i = 0; i < total_connections; i++ )
	{
		char *packed_journal = NULL;
		size_t packed_journal_len = 0;
		unsigned char *packed_rtp_buffer = NULL;
		size_t packed_rtp_buffer_len = 0;
		unsigned char *packed_rtp_payload = NULL;
		rtp_packet_t *rtp_packet = NULL;

		net_ctx_t *current_ctx = net_ctx_find_by_index( i );

		logging_printf( LOGGING_DEBUG, "midi_sender_send_single: current_ctx=%p\n", current_ctx );
		if(! current_ctx ) continue;

		logging_printf( LOGGING_DEBUG, "midi_sender_send_single: current_ctx->ssrc=0x%08x, originator_ssrc=0x%08x\n", current_ctx->ssrc, originator_ssrc );
		// If the current ctx is the originator, we don't need to send anything
		if( current_ctx->ssrc == originator_ssrc ) continue;

		// Get a journal if there is one
		net_ctx_journal_pack( current_ctx , &packed_journal, &packed_journal_len);

		if( packed_journal_len > 0 )
		{
			midi_payload_set_j( single_midi_payload );
		} else {
			midi_payload_unset_j( single_midi_payload );
		}

		// We have to pack the payload again each time because some connections may not have a journal
		// and the flag to indicate the journal being present is in the payload
		midi_payload_pack( single_midi_payload, &packed_payload, &packed_payload_len );
		logging_printf(LOGGING_DEBUG, "midi_sender_send_single: packed_payload: buffer=%p,packed_payload_len=%u packed_journal_len=%u\n", packed_payload, packed_payload_len, packed_journal_len);

		// Join the packed MIDI payload and the journal together
		packed_rtp_payload = (unsigned char *)X_MALLOC( packed_payload_len + packed_journal_len );
		memset( packed_rtp_payload, 0, packed_payload_len + packed_journal_len );
		memcpy( packed_rtp_payload, packed_payload , packed_payload_len );
		memcpy( packed_rtp_payload + packed_payload_len , packed_journal, packed_journal_len );

		rtp_packet = rtp_packet_create();
		net_ctx_increment_seq( current_ctx );

		// Transfer the connection details to the RTP packet
		net_ctx_update_rtp_fields( current_ctx , rtp_packet );

		// Add the MIDI data to the RTP packet
		rtp_packet->payload_len = packed_payload_len + packed_journal_len;

		rtp_packet->payload = (unsigned char *)X_MALLOC( rtp_packet->payload_len );
		memcpy( rtp_packet->payload, packed_rtp_payload, rtp_packet->payload_len );
		rtp_packet_dump( rtp_packet );

		// Pack the RTP data
		rtp_packet_pack( rtp_packet, &packed_rtp_buffer, &packed_rtp_buffer_len );

		net_ctx_send( current_ctx, packed_rtp_buffer, packed_rtp_buffer_len , USE_DATA_PORT );

		X_FREENULL( "packed_rtp_buffer", (void **)&packed_rtp_buffer );
		rtp_packet_destroy( &rtp_packet );

		X_FREENULL( "packed_rtp_payload", (void **)&packed_rtp_payload );
		X_FREENULL( "packed_journal", (void **)&packed_journal );

		if( ! journal_enabled )
		{
			continue;
		}
		
		switch( message_type )
		{
			case MIDI_NOTE_OFF:
			case MIDI_NOTE_ON:
				net_ctx_add_journal_note( current_ctx , midi_note );
				break;
			case MIDI_CONTROL_CHANGE:
				net_ctx_add_journal_control( current_ctx, midi_control );
				break;
			case MIDI_PROGRAM_CHANGE:	
				net_ctx_add_journal_program( current_ctx, midi_program );
				break;
			default:
				continue;
		}
	}

	// Clean up
	X_FREENULL( "packed_payload", (void **)&packed_payload );
	midi_payload_destroy( &single_midi_payload );
	switch( message_type )
	{
		case MIDI_NOTE_OFF:
		case MIDI_NOTE_ON:
			midi_note_destroy( &midi_note );
			break;
		case MIDI_CONTROL_CHANGE:
			midi_control_destroy( &midi_control );
			break;
		case MIDI_PROGRAM_CHANGE:
			midi_program_destroy( &midi_program );
			break;
		default:
			break;
	}

	// Determine if the MIDI commands need to be written out to ALSA or the local MIDI file descriptor
	output_available = ( inbound_midi_fd >= 0 );
#ifdef HAVE_ALSA
	output_available |= raveloxmidi_alsa_out_available();
#endif
	if( !output_available ) return;

	raw_buffer = (unsigned char *)X_MALLOC( 2 + command->data_len );
	if( raw_buffer )
	{       
		size_t bytes_written = 0;

		memset( raw_buffer, 0, 2 + command->data_len );

		raw_buffer[0]=command->status;

		if( command->data_len > 0 )
		{       
			memcpy( raw_buffer + 1, command->data, command->data_len );
		}       

		if( inbound_midi_fd >= 0 )
		{       
			//net_socket_send_lock();
			bytes_written = write( inbound_midi_fd, raw_buffer, 1 + command->data_len );
			//net_socket_send_unlock();
			logging_printf( LOGGING_DEBUG, "net_socket_read: inbound MIDI write(bytes=%u)\n", bytes_written );
		}       

#ifdef HAVE_ALSA
		//net_socket_send_lock();
		raveloxmidi_alsa_write( raw_buffer, 1 + command->data_len , alsa_originator_card );
		//net_socket_send_unlock();
#endif
		X_FREE( raw_buffer );
	}       
}

void midi_sender_init( void )
{
	midi_queue = data_queue_create("MIDI sender", midi_sender_handler );
	if( ! midi_queue )
	{
		logging_printf( LOGGING_ERROR, "midi_sender_init: Unable to create midi queue\n");
	}

	journal_enabled = is_yes( config_string_get("journal.write") ); 
}

