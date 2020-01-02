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

#include "midi_journal.h"
#include "utils.h"

#include "logging.h"

void journal_header_pack( journal_header_t *header , char **packed , size_t *size )
{
	char *p = NULL;

	*packed = NULL;
	*size = 0;

	if( ! header ) return;

	*packed = ( char *)malloc( JOURNAL_HEADER_PACKED_SIZE );

	if( ! packed ) return;
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

	journal_header = ( journal_header_t * ) malloc( sizeof( journal_header_t ) );
	
	if( journal_header )
	{
		memset( journal_header , 0 , sizeof( journal_header_t ) );
		journal_header->bitfield |= JOURNAL_HEADER_S_FLAG;
	}

	return journal_header;
}

void journal_header_destroy( journal_header_t **header )
{
	if( header ) FREENULL( "journal_header", (void **)header );
}

void channel_header_pack( channel_header_t *header , unsigned char **packed , size_t *size )
{
	unsigned char *p = NULL;
	uint16_t temp_header = 0;

	*packed = NULL;
	*size = 0;

	if( ! header ) return;

	*packed = ( unsigned char *)malloc( CHANNEL_HEADER_PACKED_SIZE );

	if( ! packed ) return;

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
	if( header) FREENULL( "channel_header", (void **)header);
}

channel_header_t * channel_header_create( void )
{
	channel_header_t *header = NULL;

	header = ( channel_header_t * ) malloc( sizeof( channel_header_t ) );

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

	if( channel->chapter_n )
	{
		chapter_n_pack( channel->chapter_n, &packed_chapter_n, &packed_chapter_n_size );
		channel->header->len += packed_chapter_n_size;
		logging_printf(LOGGING_DEBUG, "channel_pack: packed_chapter_n_size=%u channel->header->len=%u\n", packed_chapter_n_size,channel->header->len);
	}


	channel_header_dump( channel->header );
	channel_header_pack( channel->header, &packed_channel_header, &packed_channel_header_size );
	logging_printf(LOGGING_DEBUG, "channel_pack: packed_channel_header_size=%u channel->header->len=%u\n", packed_channel_header_size,channel->header->len);

	*packed = ( char * ) malloc( packed_channel_header_size + packed_chapter_n_size + packed_chapter_c_size + packed_chapter_p_size );

	if( ! packed ) goto channel_pack_cleanup;

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

	if( packed_chapter_n_size > 0 )
	{
		memcpy( p, packed_chapter_n, packed_chapter_n_size );
		*size += packed_chapter_n_size;
		p += packed_chapter_n_size;
	}

channel_pack_cleanup:
	FREENULL( "packed_channel_header", (void **)&packed_channel_header );
	FREENULL( "packed_chapter_n", (void **)&packed_chapter_n );
	FREENULL( "packed_chapter_c", (void **)&packed_chapter_c );
	FREENULL( "packed_chapter_p", (void **)&packed_chapter_p );
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

	FREENULL("channel", (void **) channel);
}

channel_t * channel_create( void )
{
	channel_t *new_channel = NULL;
	channel_header_t *new_header = NULL;
	
	new_channel = ( channel_t * ) malloc( sizeof( channel_t ) );

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

		packed_channel_buffer = ( char * )realloc( packed_channel_buffer, packed_channel_buffer_size + packed_channel_size );
		if( ! packed_channel_buffer ) goto journal_pack_cleanup;

		p = packed_channel_buffer + packed_channel_buffer_size;
		memset( p, 0, packed_channel_size );

		packed_channel_buffer_size += packed_channel_size;
		memcpy( p, packed_channel, packed_channel_size );

		FREENULL( "packed_channel", (void **)&packed_channel );
	}

	// Join it all together

	*packed = ( char * )malloc( packed_journal_header_size + packed_channel_buffer_size );
	p = *packed;
	memcpy( p, packed_journal_header, packed_journal_header_size );
	*size += packed_journal_header_size;
	p += packed_journal_header_size;

	memcpy( p, packed_channel_buffer, packed_channel_buffer_size );
	*size += packed_channel_buffer_size;
	
journal_pack_cleanup:
	FREENULL( "packed_channel", (void **)&packed_channel );
	FREENULL( "packed_channel_buffer", (void **)&packed_channel_buffer );
	FREENULL( "packed_journal_header", (void **)&packed_journal_header );
}

int journal_init( journal_t **journal )
{
	journal_header_t *header;
	unsigned char i;

	*journal = ( journal_t * ) malloc( sizeof ( journal_t ) );

	if( ! journal )
	{
		return -1;
	}

	header = journal_header_create();
		
	if( ! header )
	{
		free( *journal );
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

	free( *journal );
	*journal = NULL;
}

void midi_journal_add_note( journal_t *journal, uint32_t seq, midi_note_t *midi_note)
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

		// Which element
		offset = (midi_note->note) / 8;
		shift = ( (midi_note->note) - ( offset * 8 )) - 1;

		// Set low and high values;
		journal->channels[ channel ]->chapter_n->header->high = MAX( offset , journal->channels[ channel ]->chapter_n->header->high );
		journal->channels[ channel ]->chapter_n->header->low = MIN( offset , journal->channels[ channel ]->chapter_n->header->low );

		journal->channels[ channel ]->chapter_n->offbits[offset] |=  ( 1 << shift );

		return;
	}

	if( journal->channels[ channel ]->chapter_n->num_notes == MAX_CHAPTER_N_NOTES ) return;

	new_note = chapter_n_note_create();
	if(! new_note ) return;

	new_note->num = midi_note->note;
	new_note->velocity = midi_note->velocity;

	note_slot = journal->channels[ channel ]->chapter_n->num_notes;

	journal->channels[ channel ]->chapter_n->notes[note_slot] = new_note;
	journal->channels[ channel ]->chapter_n->num_notes++;
}

void midi_journal_add_control( journal_t *journal, uint32_t seq, midi_control_t *midi_control)
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


	journal->channels[ channel]->chapter_c->controller_log[ controller ].S = 1;
	journal->channels[ channel]->chapter_c->controller_log[ controller ].number = controller;
	journal->channels[ channel]->chapter_c->controller_log[ controller ].value = midi_control->controller_value;
}

void midi_journal_add_program( journal_t *journal, uint32_t seq, midi_program_t *midi_program)
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
