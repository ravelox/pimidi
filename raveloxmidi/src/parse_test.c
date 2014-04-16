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

#include <signal.h>

#include "net_applemidi.h"
#include "net_listener.h"
#include "midi_journal.h"
#include "net_connection.h"
#include "utils.h"

int main(int argc, char *argv[])
{
	/* Invitation buffer */
	// unsigned char buffer[] = { 0xff, 0xff, 0x49, 0x4e, 0x00, 0x00, 0x00, 0x02, 0x66, 0x33, 0x48, 0x73, 0x9e, 0xc3, 0xdb, 0xe3, 0x41, 0x43, 0x37, 0x00 };
	// size_t buffer_len = 20;

	/* Sync buffer */
	// unsigned char buffer[] = { 0xff, 0xff, 0x43, 0x4b, 0xa3, 0xa2, 0x02, 0xf2, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x33, 0x38, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x12, 0x84, 0xe8, 0xe8, 0x3e, 0x01, 0xe0, 0x42, 0x95, 0x7b };
	// size_t buffer_len = 36;

	/* Feedback buffer */
	// unsigned char buffer[] = { 0xff, 0xff, 0x52, 0x53, 0x1c, 0x03, 0x73, 0x94, 0xd2, 0x4b, 0xa5, 0x7b };
	// size_t buffer_len = 12;

	/* End buffer */
	unsigned char buffer[] = { 0xff, 0xff, 0x42, 0x59, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0xbd, 0x06, 0x09, 0x0b };
	size_t buffer_len = 16;

	net_applemidi_command *command;
	int result;
	unsigned char *pack_buffer;
	size_t pack_buffer_len;

	printf("Variable sizes are:\n");
	printf("\tuint8_t = %u\n", sizeof( uint8_t ) );
	printf("\tuint16_t= %lu\n", sizeof( uint16_t ) );
	printf("\tuint32_t= %lu\n", sizeof( uint32_t ) );
	printf("\tuint64_t= %lu\n", sizeof( uint64_t ) );
	printf("\tvoid *  = %lu\n", sizeof( void * ) );
	printf("\tchar *  = %lu\n", sizeof( char * ) );
	result = net_applemidi_unpack( &command, buffer, buffer_len );
	printf("\n\nUnpack result = %u\n", result);

	if( result == NET_APPLEMIDI_DONE )
	{
		net_applemidi_command_dump( command );
	}

	result = net_applemidi_pack( command, &pack_buffer, &pack_buffer_len );

	printf("\n\nPack result = %u\n", result);
	printf("Pack length = %zu\n", pack_buffer_len);
	hex_dump( pack_buffer, pack_buffer_len );
	hex_dump( buffer, buffer_len );
	
	net_applemidi_cmd_destroy( &command );

	result = net_applemidi_unpack( &command, pack_buffer, pack_buffer_len );
	if( result == NET_APPLEMIDI_DONE )
	{
		net_applemidi_command_dump( command );
		net_applemidi_cmd_destroy( &command );
		free( pack_buffer );
	}

	return 0;
}
