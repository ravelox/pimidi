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

#include "config.h"

#include "midi_journal.h"
#include "utils.h"

#include "logging.h"

void chapter_n_header_pack( chapter_n_header_t *header , unsigned char **packed , size_t *size )
{
	unsigned char *p = NULL;

	*packed = NULL;
	*size = 0;

	if( ! header ) return;

	chapter_n_header_dump( header );
	*packed = ( unsigned char *)malloc( CHAPTER_N_HEADER_PACKED_SIZE );

	if( ! *packed ) return;
	memset( *packed, 0 , CHAPTER_N_HEADER_PACKED_SIZE );

	p = *packed;

	*p |= ( ( header->B & 0x01 ) << 7 );
	*p |= ( header->len & 0x7f ) ;

	p += sizeof( char );
	*size += sizeof( char );

	*p |= ( ( header->low & 0x0f ) << 4 );
	*p |= ( header->high & 0x0f );

	*size += sizeof( char );
}

void chapter_n_header_destroy( chapter_n_header_t **header )
{
	FREENULL( "chapter_n_header",(void **)header);
}

chapter_n_header_t * chapter_n_header_create( void )
{
	chapter_n_header_t *header = NULL;

	header = ( chapter_n_header_t *) malloc( sizeof( chapter_n_header_t ) );

	if( header )
	{
		memset( header, 0 , sizeof( chapter_n_header_t) );
		header->low = 0x0f;
		header->high = 0x00;
	}

	return header;
}

void chapter_n_pack( chapter_n_t *chapter_n, unsigned char **packed, size_t *size )
{
	unsigned char *packed_header = NULL;
	unsigned char *packed_note = NULL;
	unsigned char *note_buffer = NULL;
	unsigned char *offbits_buffer = NULL;
	unsigned char *p = NULL;
	int i = 0;
	size_t header_size, note_size, note_buffer_size;
	int offbits_size;

	*packed = NULL;
	*size = 0;

	header_size = note_size = note_buffer_size = 0;

	if( ! chapter_n ) return;

	chapter_n->header->len = chapter_n->num_notes;

	chapter_n_header_pack( chapter_n->header, &packed_header, &header_size) ;

	if( packed_header )
	{
		*size += header_size;
	}

	if( chapter_n->num_notes > 0 )
	{
		note_buffer_size = CHAPTER_N_NOTE_PACKED_SIZE * chapter_n->num_notes;
		note_buffer = ( unsigned char * ) malloc( note_buffer_size );
		if( note_buffer ) 
		{
			memset( note_buffer, 0 , note_buffer_size);
			p = note_buffer;

			for( i = 0 ; i < chapter_n->num_notes ; i++ )
			{
				chapter_n_note_pack( chapter_n->notes[i], &packed_note, &note_size );
				memcpy( p, packed_note, note_size );
				p += note_size;
				*size += note_size;
				free( packed_note );
			}
		} else {
			logging_printf(LOGGING_ERROR, "chapter_n_pack: Insufficient memory to create note buffer\n");
			goto chapter_n_pack_cleanup;
		}
			
	}

	
	offbits_size = ( chapter_n->header->high - chapter_n->header->low ) + 1;
	if( offbits_size > 0 )
	{
		offbits_buffer = ( unsigned char * )malloc( offbits_size );
		p = chapter_n->offbits + chapter_n->header->low;
		memcpy( offbits_buffer, p, offbits_size );
		*size += offbits_size;
	}

	// Now pack it all together
	*packed = ( unsigned char * ) malloc( *size );

	if( ! *packed ) goto chapter_n_pack_cleanup;

	p = *packed;

	memcpy( p , packed_header, header_size );
	p += header_size;

	if( note_buffer )
	{
		memcpy( p, note_buffer, note_buffer_size );
		p+= note_buffer_size;
	}

	if( offbits_buffer )
	{
		memcpy( p, offbits_buffer, offbits_size );
	}

chapter_n_pack_cleanup:
	FREENULL( "chapter_n:packed_header", (void **)&packed_header );
	FREENULL( "chapter_n:note_buffer", (void **)&note_buffer );
	FREENULL( "chapter_n:offbits_buffer",(void **)&offbits_buffer );
}

chapter_n_t * chapter_n_create( void )
{
	chapter_n_t *chapter_n = NULL;
	unsigned int i = 0;

	chapter_n = ( chapter_n_t * ) malloc( sizeof( chapter_n_t ) );

	if( chapter_n )
	{
		chapter_n_header_t *header = chapter_n_header_create();

		memset( chapter_n, 0, sizeof( chapter_n_t ) );
		if( ! header )
		{
			free( chapter_n );
			return NULL;
		}

		chapter_n->header = header;

		chapter_n->num_notes = 0;
		for( i = 0 ; i < MAX_CHAPTER_N_NOTES ; i++ )
		{
			chapter_n->notes[i] = NULL;
		}
		
		chapter_n->offbits = ( unsigned char *)malloc( MAX_OFFBITS );
		if (! chapter_n->offbits )
		{
			chapter_n_destroy( &chapter_n );
			return NULL;
		}
			
		memset( chapter_n->offbits, 0, MAX_OFFBITS );
	}

	return chapter_n;
}

void chapter_n_destroy( chapter_n_t **chapter_n )
{
	int i;
	if( ! chapter_n ) return;
	if( ! *chapter_n ) return;

	for( i = 0 ; i < MAX_CHAPTER_N_NOTES ; i++ )
	{
		if( (*chapter_n)->notes[i] )
		{
			chapter_n_note_destroy( &( (*chapter_n)->notes[i] ) );
		}
	}

	if( (*chapter_n)->offbits )
	{
		free( (*chapter_n)->offbits );
		(*chapter_n)->offbits = NULL;
	}

	if( (*chapter_n)->header )
	{
		chapter_n_header_destroy( &( (*chapter_n)->header ) );
		(*chapter_n)->header = NULL;
	}

	free( *chapter_n );

	*chapter_n = NULL;

	return;
}

void chapter_n_header_dump( chapter_n_header_t *header )
{
	DEBUG_ONLY;
	if( ! header ) return;

	logging_printf( LOGGING_DEBUG, "Chapter N(header): B=%d len=%u low=%u high=%u\n", header->B, header->len, header->low, header->high);
}

void chapter_n_header_reset( chapter_n_header_t *header )
{
	if( ! header ) return;

	header->B = 0;
	header->len = 0;
	header->high = 0;
	header->low = 0;
}

void chapter_n_dump( chapter_n_t *chapter_n )
{
	uint16_t i = 0;

	DEBUG_ONLY;
	if( ! chapter_n ) return;

	for( i = 0 ; i < chapter_n->num_notes ; i++ )
	{
		chapter_n_note_dump( chapter_n->notes[i] );
	}
	
	for( i = chapter_n->header->low; i <= chapter_n->header->high ; i++ )
	{
		logging_printf( LOGGING_DEBUG, "Offbits[%d]=%02x\n", i, chapter_n->offbits[i]);
	}
}

void chapter_n_reset( chapter_n_t *chapter_n )
{
	uint16_t i = 0;

	if( ! chapter_n ) return;

	for( i = 0 ; i < chapter_n->num_notes ; i++ )
	{
		chapter_n_note_reset( &chapter_n->notes[i] );
	}
	chapter_n->num_notes = 0;
	memset( chapter_n->offbits, 0, MAX_OFFBITS );
	chapter_n_header_reset( chapter_n->header );
}

void chapter_n_note_pack( chapter_n_note_t *note , unsigned char **packed , size_t *size )
{
	unsigned char *p = NULL;

	*packed = NULL;
	*size = 0;

	if( ! note ) return;

	*packed = ( unsigned char *)malloc( CHAPTER_N_NOTE_PACKED_SIZE );

	if( ! *packed ) return;

	memset( *packed, 0, CHAPTER_N_NOTE_PACKED_SIZE );
	p = *packed;

	*p = ( note->S << 7 ) | ( note->num & 0x7f );

	p += sizeof( char );
	*size += sizeof( char );

	*p = ( note->Y << 7 ) | ( note->velocity & 0x7f );

	*size += sizeof( char );
}

void chapter_n_note_destroy( chapter_n_note_t **note )
{
	FREENULL( "chapter_n:note",(void **)note );
}

chapter_n_note_t * chapter_n_note_create( void )
{
	chapter_n_note_t *note = NULL;
	
	note = ( chapter_n_note_t * ) malloc( sizeof( chapter_n_note_t ) );

	if( note )
	{
		memset( note , 0 , sizeof( chapter_n_note_t ) );
		logging_printf(LOGGING_DEBUG,"chapter_n_note_create:note=%p\n", note);
	}

	return note;
}

void chapter_n_note_dump( chapter_n_note_t *note )
{
	DEBUG_ONLY;
	if( ! note ) return;

	logging_printf( LOGGING_DEBUG, "chapter_n_note: S=%d num=%u Y=%d velocity=%u\n", note->S, note->num, note->Y, note->velocity);
}

void chapter_n_note_reset( chapter_n_note_t **note )
{
	if( ! note ) return;

	(*note)->S = 0;
	(*note)->num = 0;
	(*note)->Y = 0;
	(*note)->velocity = 0;
	chapter_n_note_destroy( note );
}
