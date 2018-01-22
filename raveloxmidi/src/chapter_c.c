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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include "midi_journal.h"
#include "utils.h"

#include "logging.h"

#define MAX(a,b) ( (a) > (b) ? (a) : (b) )
#define MIN(a,b) ( (a) < (b) ? (a) : (b) )

void chaptern_header_pack( chaptern_header_t *header , unsigned char **packed , size_t *size )
{
	unsigned char *p = NULL;

	*packed = NULL;
	*size = 0;

	if( ! header ) return;

	chaptern_header_dump( header );
	*packed = ( unsigned char *)malloc( CHAPTERN_HEADER_PACKED_SIZE );

	if( ! packed ) return;
	memset( *packed, 0 , CHAPTERN_HEADER_PACKED_SIZE );

	p = *packed;

	*p |= ( ( header->B & 0x01 ) << 7 );
	*p |= ( header->len & 0x7f ) ;

	p += sizeof( char );
	*size += sizeof( char );

	*p |= ( ( header->low & 0x0f ) << 4 );
	*p |= ( header->high & 0x0f );

	*size += sizeof( char );
}

void chaptern_header_destroy( chaptern_header_t **header )
{
	FREENULL( (void **)header);
}

chaptern_header_t * chaptern_header_create( void )
{
	chaptern_header_t *header = NULL;

	header = ( chaptern_header_t *) malloc( sizeof( chaptern_header_t ) );

	if( header )
	{
		memset( header, 0 , sizeof( chaptern_header_t) );
		header->low = 0x0f;
		header->high = 0x00;
	}

	return header;
}

void chaptern_pack( chaptern_t *chaptern, char **packed, size_t *size )
{
	unsigned char *packed_header = NULL;
	char *packed_note = NULL;
	char *note_buffer = NULL;
	char *offbits_buffer = NULL;
	char *p = NULL;
	int i = 0;
	size_t header_size, note_size, note_buffer_size;
	int offbits_size;

	*packed = NULL;
	*size = 0;

	header_size = note_size = note_buffer_size = 0;

	if( ! chaptern ) return;

	chaptern->header->len = chaptern->num_notes;

	chaptern_header_pack( chaptern->header, &packed_header, &header_size) ;

	if( packed_header )
	{
		*size += header_size;
	}

	if( chaptern->num_notes > 0 )
	{
		note_buffer_size = MIDI_NOTE_PACKED_SIZE * chaptern->num_notes;
		note_buffer = ( char * ) malloc( note_buffer_size );
		if( note_buffer ) 
		{
			memset( note_buffer, 0 , note_buffer_size);
			p = note_buffer;

			for( i = 0 ; i < chaptern->num_notes ; i++ )
			{
				midi_note_pack( chaptern->notes[i], &packed_note, &note_size );
				memcpy( p, packed_note, note_size );
				p += note_size;
				*size += note_size;
				free( packed_note );
			}
		}
	}

	
	offbits_size = ( chaptern->header->high - chaptern->header->low ) + 1;
	if( offbits_size > 0 )
	{
		offbits_buffer = ( char * )malloc( offbits_size );
		p = chaptern->offbits + chaptern->header->low;
		memcpy( offbits_buffer, p, offbits_size );
		*size += offbits_size;
	}

	// Now pack it all together

	*packed = ( char * ) malloc( *size );

	if( ! packed ) goto chaptern_pack_cleanup;

	p = *packed;

	memcpy( p , packed_header, header_size );
	p += header_size;

	memcpy( p, note_buffer, note_buffer_size );
	p+= note_buffer_size;

	if( offbits_buffer )
	{
		memcpy( p, offbits_buffer, offbits_size );
	}

chaptern_pack_cleanup:
	FREENULL( (void **)&packed_header );
	FREENULL( (void **)&note_buffer );
	FREENULL( (void **)&offbits_buffer );
}

chaptern_t * chaptern_create( void )
{
	chaptern_t *chaptern = NULL;
	unsigned int i = 0;

	chaptern = ( chaptern_t * ) malloc( sizeof( chaptern_t ) );

	if( chaptern )
	{
		chaptern_header_t *header = chaptern_header_create();

		memset( chaptern, 0, sizeof( chaptern_t ) );
		if( ! header )
		{
			free( chaptern );
			return NULL;
		}

		chaptern->header = header;

		chaptern->num_notes = 0;
		for( i = 0 ; i < MAX_CHAPTERN_NOTES ; i++ )
		{
			chaptern->notes[i] = NULL;
		}
		
		chaptern->offbits = ( char *)malloc( MAX_OFFBITS );
		if (! chaptern->offbits )
		{
			chaptern_destroy( &chaptern );
			return NULL;
		}
			
		memset( chaptern->offbits, 0, MAX_OFFBITS );
	}

	return chaptern;
}

void chaptern_destroy( chaptern_t **chaptern )
{
	int i;
	if( ! chaptern ) return;
	if( ! *chaptern ) return;

	for( i = 0 ; i < MAX_CHAPTERN_NOTES ; i++ )
	{
		if( (*chaptern)->notes[i] )
		{
			midi_note_destroy( &( (*chaptern)->notes[i] ) );
		}
	}

	if( (*chaptern)->offbits )
	{
		free( (*chaptern)->offbits );
		(*chaptern)->offbits = NULL;
	}

	if( (*chaptern)->header )
	{
		chaptern_header_destroy( &( (*chaptern)->header ) );
		(*chaptern)->header = NULL;
	}

	free( *chaptern );

	*chaptern = NULL;

	return;
}

void chaptern_header_dump( chaptern_header_t *header )
{
	if( ! header ) return;

	logging_printf( LOGGING_DEBUG, "Chapter N(header): B=%d len=%u low=%u high=%u\n", header->B, header->len, header->low, header->high);
}

void chaptern_header_reset( chaptern_header_t *header )
{
	if( ! header ) return;

	header->B = 0;
	header->len = 0;
	header->high = 0;
	header->low = 0;
}

void chaptern_dump( chaptern_t *chaptern )
{
	uint16_t i = 0;

	if( ! chaptern ) return;

	chaptern_header_dump( chaptern->header );

	for( i = 0 ; i < chaptern->num_notes ; i++ )
	{
		midi_note_dump( chaptern->notes[i] );
	}
	
	for( i = chaptern->header->low; i <= chaptern->header->high ; i++ )
	{
		logging_printf( LOGGING_DEBUG, "Offbits[%d]=%02x\n", i, chaptern->offbits[i]);
	}
}

void chaptern_reset( chaptern_t *chaptern )
{
	uint16_t i = 0;

	if( ! chaptern ) return;

	for( i = 0 ; i < chaptern->num_notes ; i++ )
	{
		midi_note_reset( chaptern->notes[i] );
	}
	chaptern->num_notes = 0;
	memset( chaptern->offbits, 0, MAX_OFFBITS );
	chaptern_header_reset( chaptern->header );
}
