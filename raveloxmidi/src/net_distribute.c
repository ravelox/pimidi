/*
   This file is part of raveloxmidi.

   Copyright (C) 2019 Dave Kelly

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
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>

#include <pthread.h>

#include <errno.h>
extern int errno;

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
#include "utils.h"

#include "raveloxmidi_config.h"
#include "logging.h"

#include "raveloxmidi_alsa.h"

/* Send MIDI commands to all connections */
void net_distribute_midi( unsigned char *packet, size_t recv_len, int fd )
{
	rtp_packet_t *rtp_packet = NULL;
	unsigned char *packed_rtp_buffer = NULL;
	size_t packed_rtp_buffer_len = 0;

	midi_note_t *midi_note = NULL;
	midi_control_t *midi_control = NULL;
	midi_program_t *midi_program = NULL;
	midi_payload_t *initial_midi_payload = NULL;

	unsigned char *packed_rtp_payload = NULL;

	midi_command_t *midi_commands=NULL;
	size_t num_midi_commands=0;
	size_t midi_command_index = 0;

	char *packed_journal = NULL;
	size_t packed_journal_len = 0;
	char *description = NULL;
	enum midi_message_type_t message_type = 0;
	size_t midi_payload_len = 0;

	// Convert the buffer into a set of commands
	midi_payload_len = recv_len;
	initial_midi_payload = midi_payload_create();
	midi_payload_set_buffer( initial_midi_payload, packet, &midi_payload_len, fd );
	midi_payload_to_commands( initial_midi_payload, MIDI_PAYLOAD_STREAM, &midi_commands, &num_midi_commands, fd );
	midi_payload_destroy( &initial_midi_payload );

	for( midi_command_index = 0 ; midi_command_index < num_midi_commands ; midi_command_index++ )
	{
		midi_payload_t *single_midi_payload = NULL;
		unsigned char *packed_payload = NULL;
		size_t packed_payload_len = 0;

		/* Extract a single command as a midi payload */
		midi_command_to_payload( &(midi_commands[ midi_command_index ]), &single_midi_payload, fd );
		if( ! single_midi_payload ) continue;

		midi_command_map( &(midi_commands[ midi_command_index ]), &description, &message_type );
		midi_command_dump( &(midi_commands[ midi_command_index ]) );
		switch( message_type )
		{
			case MIDI_NOTE_OFF:
			case MIDI_NOTE_ON:
				midi_note_from_command( &(midi_commands[midi_command_index]), &midi_note);
				midi_note_dump( midi_note );
				break;
			case MIDI_CONTROL_CHANGE:	
				midi_control_from_command( &(midi_commands[midi_command_index]), &midi_control);
				midi_control_dump( midi_control );
				break;
			case MIDI_PROGRAM_CHANGE:
				midi_program_from_command( &(midi_commands[midi_command_index]), &midi_program);
				midi_program_dump( midi_program );
				break;
			default:
				break;
		}

		// Build the RTP packet
		for( net_ctx_iter_start_head() ; net_ctx_iter_has_current(); net_ctx_iter_next())
		{
			net_ctx_t *current_ctx = net_ctx_iter_current();

			logging_printf( LOGGING_DEBUG, "net_distribute: net_ctx_iter_current()=%p\n", current_ctx );
			if(! current_ctx ) continue;

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
			logging_printf(LOGGING_DEBUG, "net_distribute: packed_payload: buffer=%p,packed_payload_len=%u packed_journal_len=%u\n", packed_payload, packed_payload_len, packed_journal_len);

			// Join the packed MIDI payload and the journal together
			packed_rtp_payload = (unsigned char *)malloc( packed_payload_len + packed_journal_len );
			memset( packed_rtp_payload, 0, packed_payload_len + packed_journal_len );
			memcpy( packed_rtp_payload, packed_payload , packed_payload_len );
			memcpy( packed_rtp_payload + packed_payload_len , packed_journal, packed_journal_len );

			rtp_packet = rtp_packet_create();
			net_ctx_increment_seq( current_ctx );

			// Transfer the connection details to the RTP packet
			net_ctx_update_rtp_fields( current_ctx , rtp_packet );

			// Add the MIDI data to the RTP packet
			rtp_packet->payload_len = packed_payload_len + packed_journal_len;

			rtp_packet->payload = (unsigned char *)malloc( rtp_packet->payload_len );
			memcpy( rtp_packet->payload, packed_rtp_payload, rtp_packet->payload_len );
			rtp_packet_dump( rtp_packet );

			// Pack the RTP data
			rtp_packet_pack( rtp_packet, &packed_rtp_buffer, &packed_rtp_buffer_len );

			net_ctx_send( current_ctx, packed_rtp_buffer, packed_rtp_buffer_len , USE_DATA_PORT );

			FREENULL( "packed_rtp_buffer", (void **)&packed_rtp_buffer );
			rtp_packet_destroy( &rtp_packet );

			FREENULL( "packed_rtp_payload", (void **)&packed_rtp_payload );
			FREENULL( "packed_journal", (void **)&packed_journal );

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
		FREENULL( "packed_payload", (void **)&packed_payload );
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

		midi_command_reset( &(midi_commands[midi_command_index]) );
	}

	free( midi_commands );
}
