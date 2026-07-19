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
#include <string.h>
#include <stdint.h>

#include "config.h"

#include "midi_journal.h"
#include "utils.h"

#include "logging.h"

void journal_header_pack( const journal_header_t *header , char **packed , size_t *size )
{
	char *p = NULL;

	*packed = NULL;
	*size = 0;

	if( ! header ) return;

	*packed = ( char *)X_MALLOC( JOURNAL_HEADER_PACKED_SIZE );

	if( ! *packed ) return;
	memset( *packed, 0 , JOURNAL_HEADER_PACKED_SIZE );

	p = *packed;

	*p |= ( ( header->bitfield & 0x0f ) << 4 );
	*p |= ( ( header->totchan == 0 ? 0 : header->totchan - 1 ) & 0x0f ) ;

	p += sizeof( char );
	*size += sizeof( char );

	put_uint16( (unsigned char **)&p, header->seq, size );
}

journal_header_t * journal_header_create( void )
{
	journal_header_t *journal_header = NULL;

	journal_header = ( journal_header_t * ) X_MALLOC( sizeof( journal_header_t ) );
	
	if( journal_header )
	{
		memset( journal_header , 0 , sizeof( journal_header_t ) );
		journal_header->bitfield |= JOURNAL_HEADER_S_FLAG;
	}

	return journal_header;
}

void journal_header_destroy( journal_header_t **header )
{
	if( header ) X_FREENULL( "journal_header", (void **)header );
}

void channel_header_pack( const channel_header_t *header , unsigned char **packed , size_t *size )
{
	unsigned char *p = NULL;
	uint16_t temp_header = 0;

	*packed = NULL;
	*size = 0;

	if( ! header ) return;

	*packed = ( unsigned char *)X_MALLOC( CHANNEL_HEADER_PACKED_SIZE );

	if( ! *packed ) return;

	memset( *packed, 0, CHANNEL_HEADER_PACKED_SIZE );

	p = *packed;

	temp_header |= ( header->S << 15 );
	temp_header |= ( ( ( header->chan == 0 ? 0 : header->chan - 1 ) & 0x0f ) << 11 );
	temp_header |= ( ( header->H & 0x01 ) << 10 );
	temp_header |= ( ( header->len & 0x03ff ) );

	put_uint16( &p, temp_header, size );

	*p = header->bitfield;
	*size += sizeof( header->bitfield );
}

void channel_header_destroy( channel_header_t **header )
{
	if( header) X_FREENULL( "channel_header", (void **)header);
}

channel_header_t * channel_header_create( void )
{
	channel_header_t *header = NULL;

	header = ( channel_header_t * ) X_MALLOC( sizeof( channel_header_t ) );

	if( header )
	{
		memset( header, 0 , sizeof( channel_header_t ) );
		header->S = 1;
	}

	return header;
}

void channel_pack( channel_t *channel, char **packed, size_t *size )
{
	unsigned char *packed_channel_header = NULL;
	unsigned char *packed_chapter_n = NULL;
	unsigned char *packed_chapter_c = NULL;
	unsigned char *packed_chapter_p = NULL;
	
	size_t packed_channel_header_size = 0;
	size_t packed_chapter_n_size = 0;
	size_t packed_chapter_c_size = 0;
	size_t packed_chapter_p_size = 0;
	size_t packed_chapter_w_size = 0;
	size_t packed_chapter_t_size = 0;
	size_t packed_chapter_a_size = 0;
	unsigned char packed_chapter_w[2];
	unsigned char packed_chapter_t[1];
	unsigned char packed_chapter_a[257];
	unsigned int pressure_count = 0;

	char *p = NULL;

	*packed = NULL;
	*size = 0;

	if( ! channel ) return;

	channel->header->len = CHANNEL_HEADER_PACKED_SIZE;
	logging_printf(LOGGING_DEBUG, "channel_pack: channel->header->len=%u\n", channel->header->len);

// The order of chapters is: PCMWNETA
	if( channel->chapter_p )
	{
		chapter_p_pack( channel->chapter_p, &packed_chapter_p, &packed_chapter_p_size );
		channel->header->len += packed_chapter_p_size;
		logging_printf(LOGGING_DEBUG, "channel_pack: packed_chapter_p_size=%u channel->header->len=%u\n", packed_chapter_p_size,channel->header->len);
	}

	if( channel->chapter_c )
	{
		chapter_c_pack( channel->chapter_c, &packed_chapter_c, &packed_chapter_c_size );
		channel->header->len += packed_chapter_c_size;
		logging_printf(LOGGING_DEBUG, "channel_pack: packed_chapter_c_size=%u channel->header->len=%u\n", packed_chapter_c_size,channel->header->len);
	}
	if( channel->has_w ) {
		packed_chapter_w[0] = 0x80 | (channel->pitch_lsb & 0x7f);
		packed_chapter_w[1] = channel->pitch_msb & 0x7f;
		packed_chapter_w_size = 2;
		channel->header->len += 2;
	}

	if( channel->chapter_n )
	{
		chapter_n_pack( channel->chapter_n, &packed_chapter_n, &packed_chapter_n_size );
		channel->header->len += packed_chapter_n_size;
		logging_printf(LOGGING_DEBUG, "channel_pack: packed_chapter_n_size=%u channel->header->len=%u\n", packed_chapter_n_size,channel->header->len);
	}
	if( channel->has_t ) {
		packed_chapter_t[0] = 0x80 | (channel->channel_pressure & 0x7f);
		packed_chapter_t_size = 1;
		channel->header->len++;
	}
	if( channel->header->bitfield & CHAPTER_A ) {
		unsigned int note;
		for( note = 0; note < 128; note++ ) if( channel->poly_pressure_active[note] ) pressure_count++;
		if( pressure_count ) {
			unsigned char *a = packed_chapter_a + 1;
			packed_chapter_a[0] = 0x80 | ((pressure_count - 1) & 0x7f);
			for( note = 0; note < 128; note++ ) if( channel->poly_pressure_active[note] ) {
				*a++ = 0x80 | note; *a++ = channel->poly_pressure[note] & 0x7f;
			}
			packed_chapter_a_size = 1 + pressure_count * 2;
			channel->header->len += packed_chapter_a_size;
		}
	}


	channel_header_dump( channel->header );
	channel_header_pack( channel->header, &packed_channel_header, &packed_channel_header_size );
	logging_printf(LOGGING_DEBUG, "channel_pack: packed_channel_header_size=%u channel->header->len=%u\n", packed_channel_header_size,channel->header->len);

	*packed = ( char * ) X_MALLOC( packed_channel_header_size + packed_chapter_n_size + packed_chapter_c_size + packed_chapter_p_size + packed_chapter_w_size + packed_chapter_t_size + packed_chapter_a_size );

	if( ! *packed ) goto channel_pack_cleanup;

	p = *packed;

	memcpy( p, packed_channel_header, packed_channel_header_size );
	*size += packed_channel_header_size;
	p += packed_channel_header_size;
	
// The order of chapters is: PCMWNETA
	if( packed_chapter_p_size > 0 )
	{
		memcpy( p, packed_chapter_p, packed_chapter_p_size );
		*size += packed_chapter_p_size;
		p += packed_chapter_p_size;
	}

	if( packed_chapter_c_size > 0 )
	{
		memcpy( p, packed_chapter_c, packed_chapter_c_size );
		*size += packed_chapter_c_size;
		p += packed_chapter_c_size;
	}
	if( packed_chapter_w_size > 0 ) { memcpy( p, packed_chapter_w, packed_chapter_w_size ); *size += packed_chapter_w_size; p += packed_chapter_w_size; }

	if( packed_chapter_n_size > 0 )
	{
		memcpy( p, packed_chapter_n, packed_chapter_n_size );
		*size += packed_chapter_n_size;
		p += packed_chapter_n_size;
	}
	if( packed_chapter_t_size > 0 ) { memcpy( p, packed_chapter_t, packed_chapter_t_size ); *size += packed_chapter_t_size; p += packed_chapter_t_size; }
	if( packed_chapter_a_size > 0 ) { memcpy( p, packed_chapter_a, packed_chapter_a_size ); *size += packed_chapter_a_size; }

channel_pack_cleanup:
	X_FREENULL( "packed_channel_header", (void **)&packed_channel_header );
	X_FREENULL( "packed_chapter_n", (void **)&packed_chapter_n );
	X_FREENULL( "packed_chapter_c", (void **)&packed_chapter_c );
	X_FREENULL( "packed_chapter_p", (void **)&packed_chapter_p );
}

void channel_destroy( channel_t **channel )
{
	if( ! channel ) return;
	if( ! *channel ) return;

	if( (*channel)->chapter_n )
	{
		chapter_n_destroy( &( (*channel)->chapter_n ) );
	}

	if( (*channel)->chapter_c )
	{
		chapter_c_destroy( &( (*channel)->chapter_c ) );
	}

	if( (*channel)->chapter_p )
	{
		chapter_p_destroy( &( (*channel)->chapter_p ) );

	}

	if( (*channel)->header )
	{
		channel_header_destroy( &( (*channel)->header ) );
	}

	X_FREENULL("channel", (void **) channel);
}

channel_t * channel_create( void )
{
	channel_t *new_channel = NULL;
	channel_header_t *new_header = NULL;
	
	new_channel = ( channel_t * ) X_MALLOC( sizeof( channel_t ) );

	if( ! new_channel ) return NULL;

	new_header = channel_header_create();

	if( ! new_header )
	{
		channel_destroy( &new_channel );
		return NULL;
	}

	new_channel->header = new_header;

	new_channel->chapter_n = NULL;
	new_channel->chapter_c = NULL;
	new_channel->chapter_p = NULL;
	new_channel->has_w = 0;
	new_channel->has_t = 0;
	memset( new_channel->poly_pressure_active, 0, sizeof(new_channel->poly_pressure_active) );
	memset( new_channel->poly_pressure, 0, sizeof(new_channel->poly_pressure) );
		
	return new_channel;
}

void journal_pack( journal_t *journal, char **packed, size_t *size )
{
	char *packed_journal_header = NULL;
	size_t packed_journal_header_size = 0;
	char *packed_channel = NULL;
	size_t packed_channel_size = 0;
	char *packed_channel_buffer = NULL;	
	size_t packed_channel_buffer_size = 0;
	char *p = NULL;
	int i = 0;

	*packed = NULL;
	*size = 0;

	if( ! journal ) return;

	logging_printf( LOGGING_DEBUG, "journal_pack: journal_has_data = %s header->totchan=%u\n", ( journal_has_data( journal )  ? "YES" : "NO" ) , journal->header->totchan);
	if(  ! journal_has_data( journal ) ) return;

	journal_header_pack( journal->header, &packed_journal_header, &packed_journal_header_size );

	for( i = 0 ; i < MAX_MIDI_CHANNELS ; i++ )
	{
		if( ! journal->channels[i] ) continue;

		channel_pack( journal->channels[i], &packed_channel, &packed_channel_size );

		packed_channel_buffer = ( char * )X_REALLOC( packed_channel_buffer, packed_channel_buffer_size + packed_channel_size );
		if( ! packed_channel_buffer ) goto journal_pack_cleanup;

		p = packed_channel_buffer + packed_channel_buffer_size;
		memset( p, 0, packed_channel_size );

		packed_channel_buffer_size += packed_channel_size;
		memcpy( p, packed_channel, packed_channel_size );

		X_FREENULL( "packed_channel", (void **)&packed_channel );
	}

	// Join it all together

	*packed = ( char * )X_MALLOC( packed_journal_header_size + packed_channel_buffer_size );
	p = *packed;
	memcpy( p, packed_journal_header, packed_journal_header_size );
	*size += packed_journal_header_size;
	p += packed_journal_header_size;

	memcpy( p, packed_channel_buffer, packed_channel_buffer_size );
	*size += packed_channel_buffer_size;
	
journal_pack_cleanup:
	X_FREENULL( "packed_channel", (void **)&packed_channel );
	X_FREENULL( "packed_channel_buffer", (void **)&packed_channel_buffer );
	X_FREENULL( "packed_journal_header", (void **)&packed_journal_header );
}

int journal_init( journal_t **journal )
{
	journal_header_t *header;
	unsigned char i;

	*journal = ( journal_t * ) X_MALLOC( sizeof ( journal_t ) );

	if( ! *journal )
	{
		return -1;
	}

	header = journal_header_create();
		
	if( ! header )
	{
		X_FREE( *journal );
		*journal = NULL;
		return -1;
	}

	(*journal)->header = header;

	for( i = 0 ; i < MAX_MIDI_CHANNELS ; i++ )
	{
		(*journal)->channels[i] = NULL;
	}

	return 0;
}

void journal_destroy( journal_t **journal )
{
	unsigned int i = 0;

	if( ! journal ) return;
	if( ! *journal) return;

	for( i = 0 ; i < MAX_MIDI_CHANNELS ; i++ )
	{
		if( (*journal)->channels[i] )
		{
			channel_destroy( &( (*journal)->channels[i] ) );
		}
	}

	if( (*journal)->header )
	{
		journal_header_destroy( &( (*journal)->header ) );
		(*journal)->header = NULL;
	}

	X_FREE( *journal );
	*journal = NULL;
}

void midi_journal_add_note( journal_t *journal, uint32_t seq, const midi_note_t *midi_note)
{
	uint16_t note_slot = 0;
	chapter_n_note_t *new_note = NULL;
	unsigned char channel = 0;

	if( ! journal ) return;
	if( ! midi_note ) return;

	channel = midi_note->channel;
	if( channel >= MAX_MIDI_CHANNELS ) return;

	// Set Journal Header A and S flags
	journal->header->bitfield |= ( JOURNAL_HEADER_A_FLAG | JOURNAL_HEADER_S_FLAG );
	
	if( ! journal->channels[ channel ] )
	{
		channel_t *channel_journal = channel_create();

		if( ! channel_journal ) return;
		journal->channels[ channel ] = channel_journal;

	}

	if( ! journal->channels[ channel ]->chapter_n )
	{
		journal->channels[ channel ]->chapter_n = chapter_n_create();
	}

	journal->channels[ channel ]->header->bitfield |= CHAPTER_N;
	journal->channels[ channel ]->chapter_n->header->B = 1;

	if( journal->channels[ channel ]->header->chan != ( channel + 1 ) )
	{
		journal->channels[ channel ]->header->chan = ( channel + 1 );
		journal->header->totchan +=1;
	}

	journal->header->seq = seq;

	// Need to update NOTE OFF bits if the command is NOTE OFF
	if( midi_note->command == MIDI_COMMAND_NOTE_OFF )
	{
		uint8_t offset, shift;
		uint16_t i;

		// Which element
		offset = (midi_note->note) / 8;
		shift = 7 - ( midi_note->note - ( offset * 8 ) );

		// Set low and high values;
		journal->channels[ channel ]->chapter_n->header->high = MAX( offset , journal->channels[ channel ]->chapter_n->header->high );
		journal->channels[ channel ]->chapter_n->header->low = MIN( offset , journal->channels[ channel ]->chapter_n->header->low );

		journal->channels[ channel ]->chapter_n->offbits[offset] |=  ( 1 << shift );
		for( i = 0; i < journal->channels[channel]->chapter_n->num_notes; i++ )
		{
			if( journal->channels[channel]->chapter_n->notes[i]->num == midi_note->note )
			{
				chapter_n_note_destroy( &journal->channels[channel]->chapter_n->notes[i] );
				memmove( &journal->channels[channel]->chapter_n->notes[i],
					&journal->channels[channel]->chapter_n->notes[i+1],
					(journal->channels[channel]->chapter_n->num_notes-i-1) * sizeof(chapter_n_note_t *) );
				journal->channels[channel]->chapter_n->num_notes--;
				journal->channels[channel]->chapter_n->notes[journal->channels[channel]->chapter_n->num_notes] = NULL;
				break;
			}
		}

		return;
	}

	for( note_slot = 0; note_slot < journal->channels[channel]->chapter_n->num_notes; note_slot++ )
	{
		if( journal->channels[channel]->chapter_n->notes[note_slot]->num == midi_note->note )
		{
			journal->channels[channel]->chapter_n->notes[note_slot]->velocity = midi_note->velocity;
			return;
		}
	}
	if( journal->channels[ channel ]->chapter_n->num_notes == MAX_CHAPTER_N_NOTES ) return;

	new_note = chapter_n_note_create();
	if(! new_note ) return;

	new_note->num = midi_note->note;
	new_note->velocity = midi_note->velocity;
	journal->channels[channel]->chapter_n->offbits[midi_note->note / 8] &= ~(0x80 >> (midi_note->note % 8));

	note_slot = journal->channels[ channel ]->chapter_n->num_notes;

	journal->channels[ channel ]->chapter_n->notes[note_slot] = new_note;
	journal->channels[ channel ]->chapter_n->num_notes++;
}

void midi_journal_add_control( journal_t *journal, uint32_t seq, const midi_control_t *midi_control)
{
	unsigned char channel = 0;
	unsigned char controller = 0;

	if( ! journal ) return;
	if( ! midi_control ) return;

	channel = midi_control->channel;
	if( channel >= MAX_MIDI_CHANNELS ) return;

	controller = midi_control->controller_number;
	if( controller > (MAX_CHAPTER_C_CONTROLLERS - 1) ) return;

	// Set Journal Header A and S flags
	journal->header->bitfield |= ( JOURNAL_HEADER_A_FLAG | JOURNAL_HEADER_S_FLAG );
	
	if( ! journal->channels[ channel ] )
	{
		channel_t *channel_journal = channel_create();

		if( ! channel_journal ) return;
		journal->channels[ channel ] = channel_journal;

	}

	if( ! journal->channels[ channel ]->chapter_c )
	{
		journal->channels[ channel ]->chapter_c = chapter_c_create();
	}

	// Set flag to show that chapter C is present
	journal->channels[ channel ]->header->bitfield |= CHAPTER_C;

	if( journal->channels[ channel ]->header->chan != ( channel + 1 ) )
	{
		journal->channels[ channel ]->header->chan = ( channel + 1 );
		journal->header->totchan +=1;
	}

	journal->header->seq = seq;


	journal->channels[ channel ]->chapter_c->controller_log[ controller ].S = 1;
	journal->channels[ channel ]->chapter_c->controller_log[ controller ].number = controller;
	journal->channels[ channel ]->chapter_c->controller_log[ controller ].value = midi_control->controller_value;
}

void midi_journal_add_program( journal_t *journal, uint32_t seq, const midi_program_t *midi_program)
{
	unsigned char channel = 0;

	if( ! journal ) return;
	if( ! midi_program ) return;

	channel = midi_program->channel;
	if( channel >= MAX_MIDI_CHANNELS ) return;

	// Set Journal Header A and S flags
	journal->header->bitfield |= ( JOURNAL_HEADER_A_FLAG | JOURNAL_HEADER_S_FLAG );
	
	if( ! journal->channels[ channel ] )
	{
		channel_t *channel_journal = channel_create();

		if( ! channel_journal ) return;
		journal->channels[ channel ] = channel_journal;

	}

	if( ! journal->channels[ channel ]->chapter_p )
	{
		journal->channels[ channel ]->chapter_p = chapter_p_create();
	}

	// Set flag to show that chapter P is present
	journal->channels[ channel ]->header->bitfield |= CHAPTER_P;

	if( journal->channels[ channel ]->header->chan != ( channel + 1 ) )
	{
		journal->channels[ channel ]->header->chan = ( channel + 1 );
		journal->header->totchan +=1;
	}

	journal->header->seq = seq;

	journal->channels[ channel]->chapter_p->S = 1;
	journal->channels[ channel]->chapter_p->B = 0;
	journal->channels[ channel]->chapter_p->program = midi_program->program;
	journal->channels[ channel]->chapter_p->X =0;
	journal->channels[ channel]->chapter_p->bank_msb = 0;
	journal->channels[ channel]->chapter_p->bank_lsb = 0;
}

static channel_t *midi_journal_channel( journal_t *journal, unsigned char channel, uint32_t seq )
{
	if( !journal || channel >= MAX_MIDI_CHANNELS ) return NULL;
	journal->header->bitfield |= JOURNAL_HEADER_A_FLAG | JOURNAL_HEADER_S_FLAG;
	if( !journal->channels[channel] ) journal->channels[channel] = channel_create();
	if( !journal->channels[channel] ) return NULL;
	if( journal->channels[channel]->header->chan != channel + 1 ) {
		journal->channels[channel]->header->chan = channel + 1;
		journal->header->totchan++;
	}
	journal->header->seq = seq;
	return journal->channels[channel];
}

void midi_journal_add_command( journal_t *journal, uint32_t seq, const midi_command_t *command )
{
	channel_t *channel;
	unsigned char type;
	if( !command || command->status < 0x80 || command->status >= 0xf0 ) return;
	type = command->status & 0xf0;
	channel = midi_journal_channel( journal, command->status & 0x0f, seq );
	if( !channel ) return;
	if( type == 0xe0 && command->data_len >= 2 ) {
		channel->has_w = 1; channel->pitch_lsb = command->data[0] & 0x7f; channel->pitch_msb = command->data[1] & 0x7f;
		channel->header->bitfield |= CHAPTER_W;
	} else if( type == 0xd0 && command->data_len >= 1 ) {
		channel->has_t = 1; channel->channel_pressure = command->data[0] & 0x7f;
		channel->header->bitfield |= CHAPTER_T;
	} else if( type == 0xa0 && command->data_len >= 2 ) {
		channel->poly_pressure_active[command->data[0] & 0x7f] = 1;
		channel->poly_pressure[command->data[0] & 0x7f] = command->data[1] & 0x7f;
		channel->header->bitfield |= CHAPTER_A;
	}
}


void channel_header_dump( channel_header_t *header )
{
	DEBUG_ONLY;
	if( ! header ) return;

	logging_printf( LOGGING_DEBUG, "Channel #%d (Header: S=%d,H=%d,len=%u,bitfield=%02x)\n", header->chan, header->S, header->H, header->len, header->bitfield);
}

void channel_header_reset( channel_header_t *header )
{
	if( ! header ) return;

	header->chan = 0;
	header->S = 1;
	header->H = 0;
	header->bitfield = 0;
}

void channel_journal_dump( channel_t *channel )
{
	DEBUG_ONLY;
	if( ! channel ) return;

	logging_printf(LOGGING_DEBUG,"channel_journal_dump\n");
	channel_header_dump( channel->header );

	if( channel->header->bitfield & CHAPTER_P )
	{
		chapter_p_dump( channel->chapter_p );
	}
	if( channel->header->bitfield & CHAPTER_C )
	{
		chapter_c_dump( channel->chapter_c );
	}
	if( channel->header->bitfield & CHAPTER_N )
	{
		chapter_n_dump( channel->chapter_n );
	}
}

void channel_journal_reset( channel_t *channel )
{
	if( ! channel ) return;

	if( channel->header )
	{
		logging_printf(LOGGING_DEBUG, "channel_journal_reset( %u )\n", channel->header->chan);
	}
	chapter_n_reset( channel->chapter_n );
	chapter_c_reset( channel->chapter_c );
	chapter_p_reset( channel->chapter_p );
	channel->has_w = 0;
	channel->has_t = 0;
	memset( channel->poly_pressure_active, 0, sizeof(channel->poly_pressure_active) );
	channel_header_reset( channel->header );
}

int journal_has_data( journal_t *journal )
{
	if( ! journal ) return 0;
	if( ! journal->header ) return 0;

	return (journal->header->totchan > 0);
}

void journal_header_dump( journal_header_t *header )
{
	DEBUG_ONLY;
	if( ! header ) return;

	logging_printf( LOGGING_DEBUG, "Journal (Header: bitfield=%02x totchan=%d seq=%04x)\n", header->bitfield, header->totchan, header->seq);
	logging_printf( LOGGING_DEBUG, "Header Size = %u\n", sizeof( journal_header_t ) );
}

void journal_header_reset( journal_header_t *header )
{
	if( ! header ) return;

	header->bitfield = JOURNAL_HEADER_S_FLAG;
	header->totchan = 0;
	header->seq = 0;
}

void journal_dump( journal_t *journal )
{
	unsigned int i = 0;
	DEBUG_ONLY;
	if( ! journal ) return;

	journal_header_dump( journal->header );

	for( i = 0 ; i < MAX_MIDI_CHANNELS ; i++ )
	{
		channel_journal_dump( journal->channels[i] );
	}
}

void journal_reset( journal_t *journal )
{
	unsigned int i = 0;

	if( !journal ) return;


	for( i = 0 ; i < MAX_MIDI_CHANNELS ; i++ )
	{
		channel_journal_reset( journal->channels[i] );
	}

	journal_header_reset( journal->header );

}

static int recovery_append( unsigned char **midi, size_t *size, unsigned char status,
	unsigned char first, unsigned char second, size_t data_size )
{
	unsigned char *next = (unsigned char *)X_REALLOC( *midi, *size + 1 + data_size );
	if( !next ) return 0;
	*midi = next;
	next[(*size)++] = status;
	if( data_size > 0 ) next[(*size)++] = first & 0x7f;
	if( data_size > 1 ) next[(*size)++] = second & 0x7f;
	return 1;
}

int midi_journal_recover( const unsigned char *packed, size_t size, unsigned char **midi, size_t *midi_size )
{
	const unsigned char *p = packed;
	size_t left = size;
	unsigned int channel_count, c;
	*midi = NULL; *midi_size = 0;
	if( !packed || size < JOURNAL_HEADER_PACKED_SIZE ) return 0;
	channel_count = (packed[0] & 0x0f) + 1;
	p += JOURNAL_HEADER_PACKED_SIZE; left -= JOURNAL_HEADER_PACKED_SIZE;
	for( c = 0; c < channel_count; c++ ) {
		uint16_t h; unsigned char flags, channel; size_t channel_len, consumed = 3;
		if( left < 3 ) goto invalid;
		h = ((uint16_t)p[0] << 8) | p[1]; flags = p[2];
		channel = (h >> 11) & 0x0f; channel_len = h & 0x03ff;
		if( channel_len < 3 || channel_len > left ) goto invalid;
		p += 3;
		if( flags & CHAPTER_P ) {
			if( consumed + 3 > channel_len ) goto invalid;
			if( !recovery_append(midi,midi_size,0xc0|channel,p[0],0,1) ) goto invalid;
			p += 3; consumed += 3;
		}
		if( flags & CHAPTER_C ) {
			size_t logs, i;
			if( consumed + 1 > channel_len ) goto invalid;
			logs = (p[0] & 0x7f) + 1; p++; consumed++;
			if( consumed + logs * 2 > channel_len ) goto invalid;
			for( i=0; i<logs; i++,p+=2 ) if( !recovery_append(midi,midi_size,0xb0|channel,p[0],p[1],2) ) goto invalid;
			consumed += logs * 2;
		}
		if( flags & CHAPTER_M ) {
			size_t chapter_size;
			if( consumed + 2 > channel_len ) goto invalid;
			chapter_size = ((size_t)(p[0] & 0x03) << 8) | p[1];
			if( chapter_size < 2 || consumed + chapter_size > channel_len ) goto invalid;
			p += chapter_size; consumed += chapter_size;
		}
		if( flags & CHAPTER_W ) {
			if( consumed + 2 > channel_len ) goto invalid;
			if( !recovery_append(midi,midi_size,0xe0|channel,p[0],p[1],2) ) goto invalid;
			p += 2; consumed += 2;
		}
		if( flags & CHAPTER_N ) {
			size_t notes, offbytes, i; unsigned int low, high;
			if( consumed + 2 > channel_len ) goto invalid;
			notes = p[0] & 0x7f; low = p[1] >> 4; high = p[1] & 0x0f;
			if( notes == 127 && low == 15 && high == 0 ) notes = 128;
			offbytes = low <= high ? high-low+1 : 0; p += 2; consumed += 2;
			if( consumed + notes*2 + offbytes > channel_len ) goto invalid;
			for( i=0; i<notes; i++,p+=2 ) if( !recovery_append(midi,midi_size,0x90|channel,p[0],p[1],2) ) goto invalid;
			consumed += notes*2;
			for( i=0; i<offbytes; i++,p++ ) { unsigned int bit; for(bit=0;bit<8;bit++) if(*p & (0x80>>bit))
				if( !recovery_append(midi,midi_size,0x80|channel,(low+i)*8+bit,0,2) ) goto invalid; }
			consumed += offbytes;
		}
		if( flags & CHAPTER_E ) {
			size_t chapter_size;
			if( consumed + 1 > channel_len ) goto invalid;
			chapter_size = 1 + ((size_t)(p[0] & 0x7f) + 1) * 2;
			if( consumed + chapter_size > channel_len ) goto invalid;
			p += chapter_size; consumed += chapter_size;
		}
		if( flags & CHAPTER_T ) {
			if( consumed + 1 > channel_len ) goto invalid;
			if( !recovery_append(midi,midi_size,0xd0|channel,p[0],0,1) ) goto invalid;
			p++; consumed++;
		}
		if( flags & CHAPTER_A ) {
			size_t logs, i;
			if( consumed + 1 > channel_len ) goto invalid;
			logs = (p[0] & 0x7f) + 1; p++; consumed++;
			if( consumed + logs*2 > channel_len ) goto invalid;
			for(i=0;i<logs;i++,p+=2) if( !recovery_append(midi,midi_size,0xa0|channel,p[0],p[1],2) ) goto invalid;
			consumed += logs*2;
		}
		if( consumed != channel_len ) goto invalid;
		left -= channel_len;
	}
	return 1;
invalid:
	X_FREENULL("recovered_midi", (void **)midi); *midi_size = 0; return 0;
}
