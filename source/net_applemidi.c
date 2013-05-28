#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include <arpa/inet.h>

#include "net_applemidi.h"
#include "utils.h"

net_applemidi_inv * new_net_applemidi_inv( void )
{
	net_applemidi_inv *inv = NULL;

	inv = ( net_applemidi_inv * ) malloc( sizeof( net_applemidi_inv ) );
	
	if( inv )
	{
		inv->ssrc = 0;
		inv->version = 0;
		inv->initiator = 0;
		inv->name = NULL;
	}

	return inv;
}

net_applemidi_sync * new_net_applemidi_sync( void )
{
	net_applemidi_sync *sync = NULL;

	sync = ( net_applemidi_sync * ) malloc( sizeof( net_applemidi_sync ) );

	if( sync )
	{
		memset( sync, 0 , sizeof( net_applemidi_sync ) );
	}

	return sync;
}

int net_applemidi_cmd_destroy( net_applemidi_command **command )
{
	if( ! *command )
	{
		return NET_APPLEMIDI_DONE;
	}

	if(	(*command)->command == NET_APPLEMIDI_CMD_INV  ||
		(*command)->command == NET_APPLEMIDI_CMD_ACCEPT  ||
		(*command)->command == NET_APPLEMIDI_CMD_REJECT  ||
		(*command)->command == NET_APPLEMIDI_CMD_END  
		)
	{
		if( (*command)->data )
		{
			net_applemidi_inv *inv;
			inv = (net_applemidi_inv *)(*command)->data;
			if( inv->name )
			{
				free(inv->name);
				inv->name = NULL;
			}
			free( (*command)->data );
			(*command)->data = NULL;
		}
		free( *command );
		*command = NULL;
		return NET_APPLEMIDI_DONE;
	}

	if( (*command)->command == NET_APPLEMIDI_CMD_SYNC  ||
		(*command)->command == NET_APPLEMIDI_CMD_FEEDBACK ||
		(*command)->command == NET_APPLEMIDI_CMD_BITRATE )
	{
		if( (*command)->data )
		{
			free( (*command)->data );
			(*command)->data = NULL;
		}
		free( *command );
		*command = NULL;
		return NET_APPLEMIDI_DONE;
	}

	return NET_APPLEMIDI_DONE;
}

int net_applemidi_unpack( net_applemidi_command **command_buffer, unsigned char *in_buffer, size_t in_buffer_len)
{

	unsigned char *p;

	if( ! in_buffer )
	{
		*command_buffer = NULL;
		return NET_APPLEMIDI_NEED_DATA;
	}
		
	if( in_buffer_len < NET_APPLEMIDI_COMMAND_SIZE )
	{
		*command_buffer = NULL;
		return NET_APPLEMIDI_NEED_DATA;
	}

	p = in_buffer;

	*command_buffer = (net_applemidi_command *)malloc( sizeof( net_applemidi_command ) );

	if( ! *command_buffer )
	{
		return NET_APPLEMIDI_NO_MEMORY;
	}
	
	/* Copy the known header data */
	get_uint16( &( (*command_buffer)->signature ), &p, &in_buffer_len );
	get_uint16( &( (*command_buffer)->command ), &p, &in_buffer_len );

	fprintf( stderr, "[parse]:remaining_buffer=%zu\n", in_buffer_len );
	fprintf( stderr, "[parse]:expected=%zu\n", NET_APPLEMIDI_INV_STATIC_SIZE - sizeof( char * ) );
	fflush( stderr );

	if( 	(*command_buffer)->command == NET_APPLEMIDI_CMD_INV ||
		(*command_buffer)->command == NET_APPLEMIDI_CMD_ACCEPT ||
		(*command_buffer)->command == NET_APPLEMIDI_CMD_REJECT ||
		(*command_buffer)->command == NET_APPLEMIDI_CMD_END )
	{
		if( in_buffer_len < ( NET_APPLEMIDI_INV_STATIC_SIZE - sizeof( char * ) ) )
		{
			free( *command_buffer );
			*command_buffer = NULL;
			return NET_APPLEMIDI_NEED_DATA;
		}

		net_applemidi_inv *inv = new_net_applemidi_inv();
		if( ! inv )
		{
			free( *command_buffer );
			*command_buffer = NULL;
			return NET_APPLEMIDI_NO_MEMORY;
		}
		(*command_buffer)->data = (net_applemidi_inv *)inv;

		get_uint32( &(inv->version), &p, &in_buffer_len );
		get_uint32( &(inv->initiator), &p, &in_buffer_len );
		get_uint32( &(inv->ssrc), &p, &in_buffer_len );

		if( in_buffer_len > 0 )
		{
			inv->name = strndup( (const char *)p, in_buffer_len );
		} else {
			inv->name = NULL;
		}
		return NET_APPLEMIDI_DONE;
	}

	if( (*command_buffer)->command == NET_APPLEMIDI_CMD_FEEDBACK )
	{
		if( in_buffer_len < sizeof( net_applemidi_feedback ) )
		{
			free( *command_buffer );
			*command_buffer = NULL;
			return NET_APPLEMIDI_NEED_DATA;
		}

		net_applemidi_feedback *feedback;
		feedback = (net_applemidi_feedback *)malloc( sizeof( net_applemidi_feedback ) );

		if( ! feedback )
		{
			free(*command_buffer );
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
			free( *command_buffer );
			*command_buffer = NULL;
			return NET_APPLEMIDI_NEED_DATA;
		}

		net_applemidi_bitrate *bitrate;
		bitrate = (net_applemidi_bitrate *)malloc( sizeof( net_applemidi_bitrate ) );

		if( ! bitrate )
		{
			free(*command_buffer );
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
			free( *command_buffer );
			*command_buffer = NULL;
			return NET_APPLEMIDI_NEED_DATA;
		}

		net_applemidi_sync *sync;
		sync = (net_applemidi_sync *)malloc( sizeof (net_applemidi_sync) );
		if( ! sync )
		{
			free( *command_buffer );
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

	*out_buffer = ( unsigned char * ) malloc( NET_APPLEMIDI_COMMAND_SIZE );

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
			free( *out_buffer );
			*out_buffer = NULL;
			*out_buffer_len = 0;
			return NET_APPLEMIDI_NEED_DATA;
		}

		*out_buffer = ( unsigned char * ) realloc( *out_buffer, NET_APPLEMIDI_COMMAND_SIZE + NET_APPLEMIDI_INV_STATIC_SIZE );
		p = *out_buffer + *out_buffer_len;

		put_uint32( &p, inv->version, out_buffer_len );
		put_uint32( &p, inv->initiator, out_buffer_len );
		put_uint32( &p, inv->ssrc, out_buffer_len );

		if( inv->name )
		{
			*out_buffer = ( unsigned char * ) realloc( *out_buffer, *out_buffer_len + strlen( inv->name ) + 1 );

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
			free( *out_buffer );
			*out_buffer = NULL;
			*out_buffer_len = 0;
			return NET_APPLEMIDI_NEED_DATA;
		}

		*out_buffer = ( unsigned char * ) realloc( *out_buffer, NET_APPLEMIDI_COMMAND_SIZE + NET_APPLEMIDI_FEEDBACK_SIZE );
		p = *out_buffer + *out_buffer_len;

		put_uint32( &p, feedback->ssrc ,  out_buffer_len );
		put_uint32( &p, feedback->apple_seq ,  out_buffer_len );
	}

	if( command_buffer->command ==  NET_APPLEMIDI_CMD_BITRATE )
	{

		net_applemidi_bitrate *bitrate = ( net_applemidi_bitrate *) command_buffer->data;

		if( ! bitrate )
		{
			free( *out_buffer );
			*out_buffer = NULL;
			*out_buffer_len = 0;
			return NET_APPLEMIDI_NEED_DATA;
		}

		*out_buffer = ( unsigned char * ) realloc( *out_buffer, NET_APPLEMIDI_COMMAND_SIZE + NET_APPLEMIDI_BITRATE_SIZE );
		p = *out_buffer + *out_buffer_len;

		put_uint32( &p, bitrate->ssrc ,  out_buffer_len );
		put_uint32( &p, bitrate->limit ,  out_buffer_len );
	}

	if( command_buffer->command == NET_APPLEMIDI_CMD_SYNC )
	{
		net_applemidi_sync *sync;

		sync = (net_applemidi_sync *)command_buffer->data;

		*out_buffer = ( unsigned char * ) realloc( *out_buffer, NET_APPLEMIDI_COMMAND_SIZE + NET_APPLEMIDI_SYNC_SIZE );

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

net_applemidi_command * new_net_applemidi_command( uint16_t command )
{
	net_applemidi_command *new_command;

	new_command = ( net_applemidi_command *) malloc( sizeof( net_applemidi_command ) );
	if( new_command )
	{
		new_command->signature = 0xffff;
		new_command->command = command;
		new_command->data = NULL;
	}

	return new_command;
}
