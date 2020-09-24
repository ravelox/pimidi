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

#include "net_applemidi.h"
#include "utils.h"
#include "logging.h"

void net_applemidi_command_dump( net_applemidi_command *command)
{

	if( ! command ) return;

	switch( command->command )
	{
		case NET_APPLEMIDI_CMD_INV:
			logging_printf(LOGGING_INFO,"Command: IN\n");
			break;
		case NET_APPLEMIDI_CMD_END:
			logging_printf(LOGGING_INFO,"Command: BY\n");
			break;
		case NET_APPLEMIDI_CMD_ACCEPT:
			logging_printf(LOGGING_INFO,"Command: OK\n");
			break;
		case NET_APPLEMIDI_CMD_REJECT:
			logging_printf(LOGGING_INFO,"Command: NO\n");
			break;
		case NET_APPLEMIDI_CMD_FEEDBACK:
			logging_printf(LOGGING_INFO,"Command: RS\n");
			break;
		case NET_APPLEMIDI_CMD_BITRATE:
			logging_printf(LOGGING_INFO,"Command: RL\n");
			break;
		case NET_APPLEMIDI_CMD_SYNC:
			logging_printf(LOGGING_INFO,"Command: CK\n");
			break;
	}

	if( command->command == NET_APPLEMIDI_CMD_INV )
	{
		net_applemidi_inv	*inv_data;
		inv_data = (net_applemidi_inv *)command->data;
		logging_printf(LOGGING_DEBUG,"inv_data:version=%u,initiator=0x%08x,ssrc=0x%08x,name=\"%s\"\n",
			inv_data->version, inv_data->initiator,inv_data->ssrc,inv_data->name);
	}

	if( command->command == NET_APPLEMIDI_CMD_REJECT )
	{
		net_applemidi_inv	*reject_data;
		reject_data = (net_applemidi_inv *)command->data;
		logging_printf(LOGGING_DEBUG,"reject_data:version=%u,initiator=0x%08x,ssrc=0x%08x,name=\"%s\"\n",
			reject_data->version, reject_data->initiator,reject_data->ssrc,reject_data->name);
	}

	if( command->command == NET_APPLEMIDI_CMD_ACCEPT )
	{
		net_applemidi_inv	*ok_data;
		ok_data = (net_applemidi_inv *)command->data;
		logging_printf(LOGGING_DEBUG,"ok_data:version=%u,initiator=0x%08x,ssrc=0x%08x,name=\"%s\"\n",
			ok_data->version, ok_data->initiator,ok_data->ssrc,ok_data->name);
	}

	if( command->command == NET_APPLEMIDI_CMD_END )
	{
		net_applemidi_inv	*end_data;
		end_data = (net_applemidi_inv *)command->data;
		logging_printf(LOGGING_DEBUG,"end_data(version=%u,initiator=0x%08x,ssrc=0x%08x,name=\"%s\"\n",
			end_data->version, end_data->initiator,end_data->ssrc,end_data->name);
	}

	if( command->command == NET_APPLEMIDI_CMD_SYNC )
	{
		net_applemidi_sync	*sync_data;
		sync_data = (net_applemidi_sync *)command->data;
		logging_printf(LOGGING_DEBUG,"sync_data(ssrc=0x%08x,count=%d,padding=0x%02x%02x%02x,timestamp1=0x%016llx,timestamp2=0x%016llx,timestamp3=0x%016llx)\n",
			sync_data->ssrc, sync_data->count, sync_data->padding[0], sync_data->padding[1], sync_data->padding[2],
			sync_data->timestamp1, sync_data->timestamp2, sync_data->timestamp3);
	}

	if( command->command == NET_APPLEMIDI_CMD_FEEDBACK )
	{
		net_applemidi_feedback	*feedback_data;
		feedback_data = (net_applemidi_feedback *)command->data;
		logging_printf(LOGGING_DEBUG,"feedback_data(ssrc=0x%08x,apple_seq=%u,rtp_seq=%u)\n",
			feedback_data->ssrc, feedback_data->apple_seq, feedback_data->rtp_seq[1]);
	}
}

net_applemidi_inv * net_applemidi_inv_create( void )
{
	net_applemidi_inv *inv = NULL;

	inv = ( net_applemidi_inv * ) X_MALLOC( sizeof( net_applemidi_inv ) );
	
	if( inv )
	{
		inv->ssrc = 0;
		inv->version = 0;
		inv->initiator = 0;
		inv->name = NULL;
	}

	return inv;
}

void net_applemidi_inv_destroy( net_applemidi_inv **inv )
{
	if(! inv ) return;
	if(! *inv ) return;
	
	if( (*inv)->name )
	{
		X_FREE( (*inv)->name );
		(*inv)->name = NULL;
	}

	X_FREE( *inv );
	*inv = NULL;
}

net_applemidi_sync * net_applemidi_sync_create( void )
{
	net_applemidi_sync *sync = NULL;

	sync = ( net_applemidi_sync * ) X_MALLOC( sizeof( net_applemidi_sync ) );

	if( sync )
	{
		memset( sync, 0, sizeof( net_applemidi_sync ) );
	}

	return sync;
}

void net_applemidi_sync_destroy( net_applemidi_sync **sync )
{
	if( ! sync ) return;
	if( ! *sync ) return;

	X_FREE( *sync );
	*sync = NULL;
}

net_applemidi_feedback * net_applemidi_feedback_create( void )
{
	net_applemidi_feedback *feedback = NULL;
	
	feedback = (net_applemidi_feedback *) X_MALLOC( sizeof( net_applemidi_feedback ) );

	if( feedback )
	{
		memset( feedback , 0, sizeof( net_applemidi_feedback ) ); }

	return feedback;
}

int net_applemidi_cmd_destroy( net_applemidi_command **command )
{

	if( ! *command )
	{
		return NET_APPLEMIDI_DONE;
	}

	switch( (*command)->command )
	{
		case  NET_APPLEMIDI_CMD_INV:
		case  NET_APPLEMIDI_CMD_ACCEPT:
		case  NET_APPLEMIDI_CMD_REJECT:
		case  NET_APPLEMIDI_CMD_END:
			if( (*command)->data )
			{
				net_applemidi_inv *inv = (net_applemidi_inv *)(*command)->data;
				if( inv->name )
				{
					X_FREE( inv->name );
					inv->name = NULL;
				}
			}
			break;
		default:
			break;
	}

	if( (*command)->data )
	{
		X_FREE( (*command)->data );
		(*command)->data = NULL;
	}

	X_FREE( *command );
	*command = NULL;
	return NET_APPLEMIDI_DONE;
}

int net_applemidi_unpack( net_applemidi_command **command_buffer, unsigned char *in_buffer, size_t buffer_len)
{
	unsigned char *p = NULL;
	size_t in_buffer_len = 0;

	if( ! in_buffer )
	{
		*command_buffer = NULL;
		return NET_APPLEMIDI_NEED_DATA;
	}
		
	in_buffer_len = buffer_len;

	if( in_buffer_len < NET_APPLEMIDI_COMMAND_SIZE )
	{
		*command_buffer = NULL;
		return NET_APPLEMIDI_NEED_DATA;
	}

	p = in_buffer;

	*command_buffer = (net_applemidi_command *)X_MALLOC( sizeof( net_applemidi_command ) );

	if( ! *command_buffer )
	{
		return NET_APPLEMIDI_NO_MEMORY;
	}
	
	/* Copy the known header data */
	get_uint16( &( (*command_buffer)->signature ), &p, &in_buffer_len );
	get_uint16( &( (*command_buffer)->command ), &p, &in_buffer_len );

	if( 	(*command_buffer)->command == NET_APPLEMIDI_CMD_INV ||
		(*command_buffer)->command == NET_APPLEMIDI_CMD_ACCEPT ||
		(*command_buffer)->command == NET_APPLEMIDI_CMD_REJECT ||
		(*command_buffer)->command == NET_APPLEMIDI_CMD_END )
	{
		if( in_buffer_len < ( NET_APPLEMIDI_INV_STATIC_SIZE - sizeof( char * ) ) )
		{
			X_FREE( *command_buffer );
			*command_buffer = NULL;
			return NET_APPLEMIDI_NEED_DATA;
		}

		net_applemidi_inv *inv = net_applemidi_inv_create();
		if( ! inv )
		{
			X_FREE( *command_buffer );
			*command_buffer = NULL;
			return NET_APPLEMIDI_NO_MEMORY;
		}
		(*command_buffer)->data = (net_applemidi_inv *)inv;

		get_uint32( &(inv->version), &p, &in_buffer_len );
		get_uint32( &(inv->initiator), &p, &in_buffer_len );
		get_uint32( &(inv->ssrc), &p, &in_buffer_len );

		if( in_buffer_len > 0 )
		{
			if( p )
			{
				inv->name = ( char * )X_STRDUP( (const char *)p );
				in_buffer_len = 0;
			}
		} else {
			inv->name = NULL;
		}
		return NET_APPLEMIDI_DONE;
	}

	if( (*command_buffer)->command == NET_APPLEMIDI_CMD_FEEDBACK )
	{
		if( in_buffer_len < sizeof( net_applemidi_feedback ) )
		{
			X_FREE( *command_buffer );
			*command_buffer = NULL;
			return NET_APPLEMIDI_NEED_DATA;
		}

		net_applemidi_feedback *feedback;
		feedback = (net_applemidi_feedback *)X_MALLOC( sizeof( net_applemidi_feedback ) );

		if( ! feedback )
		{
			X_FREE(*command_buffer );
			*command_buffer = NULL;
			return NET_APPLEMIDI_NO_MEMORY;
		}
		(*command_buffer)->data = (net_applemidi_feedback *)feedback;

		get_uint32( &(feedback->ssrc), &p, &in_buffer_len );
		get_uint32( &(feedback->apple_seq), &p, &in_buffer_len );
		
		return NET_APPLEMIDI_DONE;
	}

	if( (*command_buffer)->command == NET_APPLEMIDI_CMD_BITRATE )
	{
		if( in_buffer_len < sizeof( net_applemidi_bitrate ) )
		{
			X_FREE( *command_buffer );
			*command_buffer = NULL;
			return NET_APPLEMIDI_NEED_DATA;
		}

		net_applemidi_bitrate *bitrate;
		bitrate = (net_applemidi_bitrate *)X_MALLOC( sizeof( net_applemidi_bitrate ) );

		if( ! bitrate )
		{
			X_FREE(*command_buffer );
			*command_buffer = NULL;
			return NET_APPLEMIDI_NO_MEMORY;
		}
		(*command_buffer)->data = (net_applemidi_bitrate *)bitrate;

		get_uint32( &(bitrate->ssrc), &p, &in_buffer_len );
		get_uint32( &(bitrate->limit), &p, &in_buffer_len );
		
		return NET_APPLEMIDI_DONE;
	}

	if( (*command_buffer)->command == NET_APPLEMIDI_CMD_SYNC )
	{
		if( in_buffer_len <  sizeof( net_applemidi_sync ) )
		{
			X_FREE( *command_buffer );
			*command_buffer = NULL;
			return NET_APPLEMIDI_NEED_DATA;
		}

		net_applemidi_sync *sync;
		sync = (net_applemidi_sync *)X_MALLOC( sizeof (net_applemidi_sync) );
		if( ! sync )
		{
			X_FREE( *command_buffer );
			*command_buffer = NULL;
			return NET_APPLEMIDI_NO_MEMORY;
		}
		(*command_buffer)->data = (net_applemidi_sync *)sync;

		get_uint32( &(sync->ssrc), &p, &in_buffer_len );

		memcpy( &(sync->count), p, 1 );
		p += 1;
		in_buffer_len--;

		memcpy( &(sync->padding), p, sizeof(sync->padding) );
		p += sizeof( sync->padding );
		in_buffer_len -= sizeof( sync->padding );

		get_uint64( &(sync->timestamp1), &p, &in_buffer_len );
		get_uint64( &(sync->timestamp2), &p, &in_buffer_len );
		get_uint64( &(sync->timestamp3), &p, &in_buffer_len );

		return NET_APPLEMIDI_DONE;
	}

	return NET_APPLEMIDI_DONE;
}

int net_applemidi_pack( net_applemidi_command *command_buffer, unsigned char **out_buffer, size_t *out_buffer_len )
{
	unsigned char *p;

	*out_buffer_len = 0;

	if( ! command_buffer )
	{
		return NET_APPLEMIDI_NEED_DATA;
	}

	*out_buffer = ( unsigned char * ) X_MALLOC( NET_APPLEMIDI_COMMAND_SIZE );

	if( ! *out_buffer )
	{
		return NET_APPLEMIDI_NO_MEMORY;
	}
	
	p = *out_buffer;

	put_uint16( &p , command_buffer->signature, out_buffer_len );
	put_uint16( &p , command_buffer->command, out_buffer_len );

	if(	command_buffer->command == NET_APPLEMIDI_CMD_INV  ||
		command_buffer->command == NET_APPLEMIDI_CMD_ACCEPT ||
		command_buffer->command == NET_APPLEMIDI_CMD_REJECT ||
		command_buffer->command == NET_APPLEMIDI_CMD_END )
	{

		net_applemidi_inv *inv = ( net_applemidi_inv *) command_buffer->data;

		if( ! inv )
		{
			X_FREE( *out_buffer );
			*out_buffer = NULL;
			*out_buffer_len = 0;
			return NET_APPLEMIDI_NEED_DATA;
		}

		*out_buffer = ( unsigned char * ) X_REALLOC( *out_buffer, NET_APPLEMIDI_COMMAND_SIZE + NET_APPLEMIDI_INV_STATIC_SIZE );
		p = *out_buffer + *out_buffer_len;

		put_uint32( &p, inv->version, out_buffer_len );
		put_uint32( &p, inv->initiator, out_buffer_len );
		put_uint32( &p, inv->ssrc, out_buffer_len );

		if( inv->name )
		{
			*out_buffer = ( unsigned char * ) X_REALLOC( *out_buffer, *out_buffer_len + strlen( inv->name ) + 1 );

			if( ! *out_buffer )
			{
				*out_buffer_len = 0;
				return NET_APPLEMIDI_NO_MEMORY;
			}

			p = *out_buffer + *out_buffer_len;
			memcpy( p , inv->name, strlen( inv->name ) );
			p += strlen( inv->name );
			*out_buffer_len += strlen( inv->name );
			*p = '\0';
			*out_buffer_len += 1;
		}
	}

	if( command_buffer->command ==  NET_APPLEMIDI_CMD_FEEDBACK )
	{

		net_applemidi_feedback *feedback = ( net_applemidi_feedback *) command_buffer->data;

		if( ! feedback )
		{
			X_FREE( *out_buffer );
			*out_buffer = NULL;
			*out_buffer_len = 0;
			return NET_APPLEMIDI_NEED_DATA;
		}

		*out_buffer = ( unsigned char * ) X_REALLOC( *out_buffer, NET_APPLEMIDI_COMMAND_SIZE + NET_APPLEMIDI_FEEDBACK_SIZE );
		p = *out_buffer + *out_buffer_len;

		put_uint32( &p, feedback->ssrc ,  out_buffer_len );
		put_uint32( &p, feedback->apple_seq ,  out_buffer_len );
	}

	if( command_buffer->command ==  NET_APPLEMIDI_CMD_BITRATE )
	{

		net_applemidi_bitrate *bitrate = ( net_applemidi_bitrate *) command_buffer->data;

		if( ! bitrate )
		{
			X_FREE( *out_buffer );
			*out_buffer = NULL;
			*out_buffer_len = 0;
			return NET_APPLEMIDI_NEED_DATA;
		}

		*out_buffer = ( unsigned char * ) X_REALLOC( *out_buffer, NET_APPLEMIDI_COMMAND_SIZE + NET_APPLEMIDI_BITRATE_SIZE );
		p = *out_buffer + *out_buffer_len;

		put_uint32( &p, bitrate->ssrc ,  out_buffer_len );
		put_uint32( &p, bitrate->limit ,  out_buffer_len );
	}

	if( command_buffer->command == NET_APPLEMIDI_CMD_SYNC )
	{
		net_applemidi_sync *sync;

		sync = (net_applemidi_sync *)command_buffer->data;

		*out_buffer = ( unsigned char * ) X_REALLOC( *out_buffer, NET_APPLEMIDI_COMMAND_SIZE + NET_APPLEMIDI_SYNC_SIZE );

		if( ! *out_buffer )
		{
			*out_buffer_len = 0;
			return NET_APPLEMIDI_NO_MEMORY;
		}

		p = *out_buffer + *out_buffer_len;

		put_uint32( &p , sync->ssrc, out_buffer_len );

		memcpy( p, &(sync->count), 1 );
		p += 1;
		*out_buffer_len += 1;

		memcpy( p, &(sync->padding), sizeof(sync->padding) );
		p += sizeof( sync->padding );
		*out_buffer_len += sizeof( sync->padding );

		put_uint64( &p, sync->timestamp1 , out_buffer_len );
		put_uint64( &p, sync->timestamp2 , out_buffer_len );
		put_uint64( &p, sync->timestamp3 , out_buffer_len );

	}

	return NET_APPLEMIDI_DONE;
}

net_applemidi_command * net_applemidi_cmd_create( uint16_t command )
{
	net_applemidi_command *new_command = NULL;

	new_command = ( net_applemidi_command *) X_MALLOC( sizeof( net_applemidi_command ) );
	if( new_command )
	{
		new_command->signature = 0xffff;
		new_command->command = command;
		new_command->data = NULL;
	}

	return new_command;
}
