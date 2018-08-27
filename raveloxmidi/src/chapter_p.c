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

chapter_p_t *chapter_p_create( void )
{
	chapter_p_t *new_chapter_p;

	new_chapter_p = (chapter_p_t *)malloc( sizeof( chapter_p_t ) );

	if( ! new_chapter_p )
	{
		logging_printf(LOGGING_ERROR,"chapter_p_create: Unable to allocate memory for chapter_p journal\n");
		return NULL;
	}

	memset( new_chapter_p, 0, sizeof( chapter_p_t ) );

	return new_chapter_p;
}

void chapter_p_pack( chapter_p_t *chapter_p, unsigned char **packed, size_t *size )
{
	if( ! chapter_p ) return;
	*size = 0;

	// Sec A.2 of RFC6295.txt
	// Chapter has a fixed size of 24 bits
	*packed = (unsigned char *)malloc( CHAPTER_P_PACKED_SIZE ); 

	if( ! *packed )
	{
		logging_printf(LOGGING_ERROR,"chapter_p_pack: Unable to allocate memory for packed chapter_p journal\n");
		return;
	}

	memset(*packed, 0, CHAPTER_P_PACKED_SIZE);
	*size = CHAPTER_P_PACKED_SIZE;
	(*packed)[0] = ( (chapter_p->S & 0x01) << 7 ) | (chapter_p->program & 0x7f);
	(*packed)[1] = ( (chapter_p->B & 0x01) << 7 ) | (chapter_p->bank_msb & 0x7f);
	(*packed)[2] = ( (chapter_p->X & 0x01) << 7 ) | (chapter_p->bank_lsb & 0x7f);
}

void chapter_p_unpack( unsigned char *packed, size_t size, chapter_p_t **chapter_p)
{
	*chapter_p = NULL;
	if( !packed ) return;
	if( size < CHAPTER_P_PACKED_SIZE ) return;
	
	*chapter_p = chapter_p_create();
	if( ! *chapter_p )
	{
		logging_printf(LOGGING_ERROR, "chapter_p_unpack: Unable to create chapter_p from buffer\n");
		return;
	}

	(*chapter_p)->S = (packed[0] & 0x80) >> 7;
	(*chapter_p)->program = packed[0] & 0x7f;
	(*chapter_p)->B = (packed[1] & 0x80) >> 7;
	(*chapter_p)->bank_msb = packed[1] & 0x7f;
	(*chapter_p)->X = (packed[2] & 0x80) >> 7;
	(*chapter_p)->bank_lsb = packed[2] & 0x7f;
}

void chapter_p_destroy( chapter_p_t **chapter_p )
{
	if( ! chapter_p ) return;
	if( ! *chapter_p ) return;

	FREENULL( "chapter_p",(void **)chapter_p );
}

void chapter_p_dump( chapter_p_t *chapter_p )
{
	DEBUG_ONLY;
	logging_printf(LOGGING_DEBUG," chapter_p: S=%u,program=%u,B=%u,msb=%u,X=%u,lsb=%u\n",
		chapter_p->S, chapter_p->program, chapter_p->B, chapter_p->bank_msb, chapter_p->X, chapter_p->bank_lsb);
}

void chapter_p_reset( chapter_p_t *chapter_p )
{
	if( ! chapter_p ) return;

	chapter_p->S = 0;
	chapter_p->program = 0;
	chapter_p->B = 0;
	chapter_p->bank_msb = 0;
	chapter_p->X = 0;
	chapter_p->bank_lsb = 0;
}
