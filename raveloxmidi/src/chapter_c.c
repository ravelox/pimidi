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

chapter_c_t *chapter_c_create( void )
{
	uint8_t index = 0;
	chapter_c_t *new_chapter_c = NULL;

	new_chapter_c = (chapter_c_t *)malloc( sizeof( chapter_c_t ) );
	if( ! new_chapter_c )
	{
		logging_printf(LOGGING_ERROR, "chapter_c_create: Unable to allocate memory to create chapter_c log\n");
		return NULL;
	}
	
	memset( new_chapter_c, 0, sizeof( chapter_c_t ) );
	new_chapter_c->S = 1;

	for( index = 0 ; index < MAX_CHAPTER_C_CONTROLLERS; index++ )
	{
		new_chapter_c->controller_log[ index ].number = 255;
	}

	return new_chapter_c;
}

void chapter_c_unpack( unsigned char *packed, size_t size, chapter_c_t **chapter_c )
{
	unsigned char *p = NULL;
	uint8_t index = 0;
	size_t current_size;

	*chapter_c = NULL;
	if(! packed )
	{
		return;
	}

	// Buffer size must be at least 1 byte long to determine length
	if( size == 0 )
	{
		return;
	}

	*chapter_c = chapter_c_create();
	if( ! *chapter_c )
	{
		logging_printf(LOGGING_ERROR, "chapter_c_unpack: Unable to unpack buffer\n");
		return;
	}

	p = packed;
	(*chapter_c)->S = ( (*p) & 0x80 ) >> 7;
	(*chapter_c)->len = (*p) & 0x7f;
	current_size = size;

	// Loop through controller logs in the buffer
	do
	{
		if( current_size < PACKED_CONTROLLER_LOG_SIZE )
		{
			logging_printf( LOGGING_ERROR, "chapter_c_unpack: Unable to unpack controller log %u. Expected %u, got %d\n", index, PACKED_CONTROLLER_LOG_SIZE, current_size );
			break;
		}

		// Get the controller number
		index = (*p) & 0x7f;

		(*chapter_c)->controller_log[index].S = ( (*p)  & 0x80 ) >> 7;
		(*chapter_c)->controller_log[index].number = index;
		p++;
		current_size--;

		(*chapter_c)->controller_log[index].A = ( (*p) & 0x80 ) >> 7;
		if( (*chapter_c)->controller_log[index].A == 1 )
		{
			(*chapter_c)->controller_log[index].T = ( (*p) & 0x40 ) >> 6;
			(*chapter_c)->controller_log[index].value = (*p) & 0x3f;
		} else {
			(*chapter_c)->controller_log[index].T = 0;
			(*chapter_c)->controller_log[index].value = (*p) & 0x7f;
		}
		current_size--;
		p++;
	} while( current_size >= PACKED_CONTROLLER_LOG_SIZE );
}

void chapter_c_pack( chapter_c_t *chapter_c, unsigned char **packed, size_t *size )
{
	uint8_t index, current_size;
	unsigned char *p = NULL;

	*packed = NULL;
	*size = 0;

	if( ! chapter_c ) return;

	// Ensure the len field is correct
	chapter_c->len = 0;
	for( index=0; index < MAX_CHAPTER_C_CONTROLLERS; index++ )
	{
		if( chapter_c->controller_log[index].number == index )
		{
			chapter_c->len++;
		}
	}

	logging_printf(LOGGING_DEBUG, "chapter_c_pack: chapter_c->len=%u\n", chapter_c->len );
	if( chapter_c->len == 0 ) return;

	*size = PACKED_CHAPTER_C_HEADER_SIZE + ( (chapter_c->len) * PACKED_CONTROLLER_LOG_SIZE );
	*packed = (unsigned char *)malloc( *size );
	
	if(! *packed )
	{
		*size = 0;
		logging_printf(LOGGING_ERROR,"chapter_c_pack: Unable to allocate memory for packed chapter_c\n");
		return;
	}

	memset( *packed, 0, *size );
	current_size = *size;
	p = *packed;
		
	// Pack the header
	// LENGTH is number of controllers - 1
	*p = ( chapter_c->S  << 7 ) | ( ( chapter_c->len - 1 ) & 0x7f );
	logging_printf(LOGGING_DEBUG,"chapter_c_pack: header = 0x%02x\n", *p);

	p++;
	current_size--;

	index = 0;

	// Loop through the controllers
	while( ( current_size >= PACKED_CONTROLLER_LOG_SIZE ) && ( index < MAX_CHAPTER_C_CONTROLLERS ) )
	{
		// Short-circuit if the controller isn't active
		if( chapter_c->controller_log[index].number == index )
		{
			controller_log_dump( &(chapter_c->controller_log[index]) );
			*p = ( chapter_c->controller_log[index].S << 7 ) | ( chapter_c->controller_log[index].number & 0x7f );
			logging_printf(LOGGING_DEBUG,"chapter_c_pack: controller = 0x%02x\n", *p);
			p++;
			current_size--;
		
			*p = chapter_c->controller_log[index].A << 7 ;
			if( chapter_c->controller_log[index].A == 1 )
			{
				*p |= ( chapter_c->controller_log[index].T << 6 );
				*p |= ( chapter_c->controller_log[index].value & 0x3f );
			} else {
				*p |= ( chapter_c->controller_log[index].value  & 0x7f );
			}
			logging_printf(LOGGING_DEBUG,"chapter_c_pack: value = 0x%02x\n", *p);
			p++;
			current_size--;
		}
		index++;
	}
}

void chapter_c_destroy( chapter_c_t **chapter_c )
{
	if( ! chapter_c ) return;
	if( ! *chapter_c ) return;

	FREENULL( "chapter_c",(void **) &(*chapter_c) );
}

void chapter_c_reset( chapter_c_t *chapter_c )
{
	uint8_t index;
	if( !chapter_c ) return;
	
	memset( chapter_c, 0, sizeof(chapter_c_t) );
	chapter_c->S = 1; 
	for( index = 0 ; index < MAX_CHAPTER_C_CONTROLLERS; index++ )
	{
		chapter_c->controller_log[ index ].S = 1;
		chapter_c->controller_log[ index ].number = 255;
	}
}

void chapter_c_dump( chapter_c_t *chapter_c )
{
	uint8_t index;
	DEBUG_ONLY;
	if(! chapter_c) return;

	logging_printf(LOGGING_DEBUG, "chapter_c: S=%u,len=%u\n", chapter_c->S, chapter_c->len);

	for( index = 0; index < MAX_CHAPTER_C_CONTROLLERS; index++ )
	{
		if( chapter_c->controller_log[index].number == index )
		{
			controller_log_dump( &(chapter_c->controller_log[index]) );
		}
	}
}

controller_log_t *controller_log_create( void )
{
	controller_log_t *new_controller_log = NULL;

	new_controller_log = (controller_log_t *)malloc( sizeof(controller_log_t ) );
	if( ! new_controller_log )
	{
		logging_printf( LOGGING_ERROR,"controller_log_create: Unable to allocate memory for new controller_log_t\n");
		return NULL;
	}

	memset( new_controller_log, 0, sizeof(controller_log_t ));
	new_controller_log->S = 1;
	new_controller_log->number=255;

	return new_controller_log;
}

void controller_log_destroy( controller_log_t **controller_log )
{
	if( ! controller_log ) return;
	if( ! *controller_log) return;

	FREENULL( "controller_log",(void **)controller_log);
}
void controller_log_reset( controller_log_t *controller_log )
{
	if(! controller_log ) return;
	memset( controller_log, 0, sizeof(controller_log_t) );
	controller_log->S = 1;
	controller_log->number = 255;
}

void controller_log_dump( controller_log_t *controller_log )
{
	DEBUG_ONLY;
	if( ! controller_log ) return;

	logging_printf(LOGGING_DEBUG, "controller_log: S=%u,number=%u,A=%u,T=%u,value=%u\n",
		controller_log->S, controller_log->number, controller_log->A,controller_log->T,controller_log->value);
}
