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

void chapterc_header_pack( chapterc_header_t *header , unsigned char **packed , size_t *size )
{
	unsigned char *p = NULL;

	*packed = NULL;
	*size = 0;

	if( ! header ) return;

	chapterc_header_dump( header );
	*packed = ( unsigned char *)malloc( CHAPTERC_HEADER_PACKED_SIZE );

	if( ! packed ) return;
	memset( *packed, 0 , CHAPTERC_HEADER_PACKED_SIZE );

	p = *packed;

	*p |= ( ( header->S & 0x01 ) << 7 );
	*p |= ( header->len & 0x7f ) ;

	*size += sizeof( char );
}

void chapterc_header_destroy( chapterc_header_t **header )
{
	FREENULL( (void **)header);
}

chapterc_header_t * chapterc_header_create( void )
{
	chapterc_header_t *header = NULL;

	header = ( chapterc_header_t *) malloc( sizeof( chapterc_header_t ) );

	if( header )
	{
		memset( header, 0 , sizeof( chapterc_header_t) );
	}

	return header;
}

void chapterc_pack( chapterc_t *chapterc, unsigned char **packed, size_t *size )
{
	unsigned char *packed_header = NULL;
	size_t header_size;

	*packed = NULL;
	*size = 0;

	if( ! chapterc ) return;

	if( chapterc->num_controllers == 0 ) return;

	*packed = (unsigned char *) malloc( chapterc->num_controllers * CHAPTER_C_PACKED_SIZE );

	if( ! *packed ) return;

	chapterc->header->len = chapterc->num_controllers;

	chapterc_header_pack( chapterc->header, &packed_header, &header_size) ;

	if( packed_header )
	{
		*size += header_size;
	}

/*
	if( chapterc->num_notes > 0 )
	{
		note_buffer_size = MIDI_NOTE_PACKED_SIZE * chapterc->num_notes;
		note_buffer = ( char * ) malloc( note_buffer_size );
		if( note_buffer ) 
		{
			memset( note_buffer, 0 , note_buffer_size);
			p = note_buffer;

			for( i = 0 ; i < chapterc->num_notes ; i++ )
			{
				midi_note_pack( chapterc->notes[i], &packed_note, &note_size );
				memcpy( p, packed_note, note_size );
				p += note_size;
				*size += note_size;
				free( packed_note );
			}
		}
	}

	
	offbits_size = ( chapterc->header->high - chapterc->header->low ) + 1;
	if( offbits_size > 0 )
	{
		offbits_buffer = ( char * )malloc( offbits_size );
		p = chapterc->offbits + chapterc->header->low;
		memcpy( offbits_buffer, p, offbits_size );
		*size += offbits_size;
	}

	// Now pack it all together

	*packed = ( char * ) malloc( *size );

	if( ! packed ) goto chapterc_pack_cleanup;

	p = *packed;

	memcpy( p , packed_header, header_size );
	p += header_size;

	memcpy( p, note_buffer, note_buffer_size );
	p+= note_buffer_size;

	if( offbits_buffer )
	{
		memcpy( p, offbits_buffer, offbits_size );
	}

chapterc_pack_cleanup:
	FREENULL( (void **)&packed_header );
	FREENULL( (void **)&note_buffer );
	FREENULL( (void **)&offbits_buffer );
*/
}

chapterc_t * chapterc_create( void )
{
	chapterc_t *chapterc = NULL;
/*
	unsigned int i = 0;

	chapterc = ( chapterc_t * ) malloc( sizeof( chapterc_t ) );

	if( chapterc )
	{
		chapterc_header_t *header = chapterc_header_create();

		memset( chapterc, 0, sizeof( chapterc_t ) );
		if( ! header )
		{
			free( chapterc );
			return NULL;
		}

		chapterc->header = header;

		chapterc->num_notes = 0;
		for( i = 0 ; i < MAX_CHAPTERN_NOTES ; i++ )
		{
			chapterc->notes[i] = NULL;
		}
		
		chapterc->offbits = ( char *)malloc( MAX_OFFBITS );
		if (! chapterc->offbits )
		{
			chapterc_destroy( &chapterc );
			return NULL;
		}
			
		memset( chapterc->offbits, 0, MAX_OFFBITS );
	}
*/

	return chapterc;
}

void chapterc_destroy( chapterc_t **chapterc )
{
/*
	int i;
	if( ! chapterc ) return;
	if( ! *chapterc ) return;

	for( i = 0 ; i < MAX_CHAPTERN_NOTES ; i++ )
	{
		if( (*chapterc)->notes[i] )
		{
			midi_note_destroy( &( (*chapterc)->notes[i] ) );
		}
	}

	if( (*chapterc)->offbits )
	{
		free( (*chapterc)->offbits );
		(*chapterc)->offbits = NULL;
	}

	if( (*chapterc)->header )
	{
		chapterc_header_destroy( &( (*chapterc)->header ) );
		(*chapterc)->header = NULL;
	}

	free( *chapterc );

	*chapterc = NULL;

	return;
*/
}

void chapterc_header_dump( chapterc_header_t *header )
{
/*
	if( ! header ) return;

	logging_printf( LOGGING_DEBUG, "Chapter N(header): B=%d len=%u low=%u high=%u\n", header->B, header->len, header->low, header->high);
*/
}

void chapterc_header_reset( chapterc_header_t *header )
{
/*
	if( ! header ) return;

	header->B = 0;
	header->len = 0;
	header->high = 0;
	header->low = 0;
*/
}

void chapterc_dump( chapterc_t *chapterc )
{
/*
	uint16_t i = 0;

	if( ! chapterc ) return;

	chapterc_header_dump( chapterc->header );

	for( i = 0 ; i < chapterc->num_notes ; i++ )
	{
		midi_note_dump( chapterc->notes[i] );
	}
	
	for( i = chapterc->header->low; i <= chapterc->header->high ; i++ )
	{
		logging_printf( LOGGING_DEBUG, "Offbits[%d]=%02x\n", i, chapterc->offbits[i]);
	}
*/
}

void chapterc_reset( chapterc_t *chapterc )
{
/*
	uint16_t i = 0;

	if( ! chapterc ) return;

	for( i = 0 ; i < chapterc->num_notes ; i++ )
	{
		midi_note_reset( chapterc->notes[i] );
	}
	chapterc->num_notes = 0;
	memset( chapterc->offbits, 0, MAX_OFFBITS );
	chapterc_header_reset( chapterc->header );
*/
}
