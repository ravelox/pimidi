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

#include "midi_command.h"
#include "utils.h"

void midi_command_destroy( midi_command_t **command )
{
	if( ! command ) return;
	if( ! *command ) return;

	FREENULL( (void **)command );
}

midi_command_t * midi_command_create( void )
{
	midi_command_t *command = NULL;

	command = ( midi_command_t * )malloc( sizeof( midi_command_t ) );

	if( ! command ) return NULL;

	memset( command, 0, sizeof( midi_command_t ) );

	return command;
}
