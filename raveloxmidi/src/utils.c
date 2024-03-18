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
#include <ctype.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/time.h>

#include <errno.h>
extern int errno;

#include "config.h"
#include "logging.h"

#define INSIDE_UTILS

#include "utils.h"

static pthread_mutex_t utils_thread_lock;
static unsigned int random_seed = 0;

static pthread_mutex_t utils_diag_mem_lock;
int utils_mem_tracking_enabled = 0;
static char *utils_mem_tracking_filename = NULL;
static char *utils_timestamp_string = NULL;
#define	UTILS_MEM_TRACKING_ENV	"RAVELOXMIDI_MEM_FILE"

int utils_pthread_enabled = 0;
static char *utils_pthread_filename = NULL;
static FILE *utils_pthread_fp = NULL;
#define UTILS_PTHREAD_TRACKING_ENV	"RAVELOXMIDI_PTHREAD_FILE"

extern int errno;

uint64_t ntohll(const uint64_t value)
{
	enum { TYP_INIT, TYP_SMLE, TYP_BIGE };
      	union
	{
		uint64_t ull;
		uint8_t  c[8];
	} x;

	// Test if on Big Endian system.
	static int typ = TYP_INIT;

	if (typ == TYP_INIT)
	{
		x.ull = 0x01;
		typ = (x.c[7] == 0x01) ? TYP_BIGE : TYP_SMLE;
	}

	// System is Big Endian; return value as is.
	if (typ == TYP_BIGE)
	{
		return value;
	}

	// else convert value to Big Endian
	x.ull = value;
	int8_t c = 0;
	c = x.c[0]; x.c[0] = x.c[7]; x.c[7] = c;
	c = x.c[1]; x.c[1] = x.c[6]; x.c[6] = c;
	c = x.c[2]; x.c[2] = x.c[5]; x.c[5] = c;
	c = x.c[3]; x.c[3] = x.c[4]; x.c[4] = c;
	return x.ull;
}

uint64_t htonll(const uint64_t value)
{
	return ntohll(value);
}

void get_uint16( void *dest, unsigned char **src, size_t *len )
{
	uint16_t value = 0;

	if(! dest )
	{
		return;
	}

	if( ! src || ! *src )
	{
		return;
	}

	memcpy( &value, *src, sizeof(uint16_t) );
	*src += sizeof( uint16_t );
	*len -= sizeof( uint16_t );
	value = ntohs( value );
	memcpy( dest, &value, sizeof(uint16_t) );
}

void put_uint16( unsigned char **dest, uint16_t src, size_t *len )
{
	uint16_t value = 0;

	if(! *dest )
	{
		return;
	}

	value = htons( src );
	memcpy( *dest, &value, sizeof(uint16_t) );
	*dest += sizeof( uint16_t );
	*len += sizeof( uint16_t );
}

void put_uint32( unsigned char **dest, uint32_t src, size_t *len )
{
	uint32_t value = 0;

	if(! *dest )
	{
		return;
	}

	value = htonl( src );
	memcpy( *dest, &value, sizeof(uint32_t) );
	*dest += sizeof( uint32_t );
	*len += sizeof( uint32_t );
}

void put_uint64( unsigned char **dest, uint64_t src, size_t *len )
{
	uint64_t value = 0;

	if(! *dest )
	{
		return;
	}

	value = htonll( src );
	memcpy( *dest, &value, sizeof(uint64_t) );
	*dest += sizeof( uint64_t );
	*len += sizeof( uint64_t );
}

void get_uint32( void *dest, unsigned char **src, size_t *len )
{
	uint32_t value = 0;

	if(! dest )
	{
		return;
	}

	if( ! src || ! *src )
	{
		return;
	}

	memcpy( &value, *src, sizeof(uint32_t) );
	*src += sizeof( uint32_t );
	*len -= sizeof( uint32_t );
	value = ntohl( value );
	memcpy( dest, &value, sizeof(uint32_t) );
}

void get_uint64( void *dest, unsigned char **src, size_t *len )
{
	uint64_t value = 0;

	if(! dest )
	{
		return;
	}

	if( ! src || ! *src )
	{
		return;
	}

	memcpy( &value, *src, sizeof(uint64_t) );
	*src += sizeof( uint64_t );
	*len -= sizeof( uint64_t );
	value = ntohll( value );
	memcpy( dest, &value, sizeof(uint64_t) );
}

void hex_dump( unsigned char *buffer, size_t len )
{
	size_t i = 0 ;
	unsigned char c = 0 ;

/* Only do a hex dump at debug level */
	DEBUG_ONLY;
/* And if it is enabled */
	HEX_DUMP_ENABLED;

	utils_lock();
	logging_prefix_disable();

	logging_printf(LOGGING_DEBUG, "hex_dump(%p , %u)\n", buffer, len );
	if( !buffer || len <= 0)
	{
		utils_unlock();
		return;
	}

	for( i = 0 ; i < len ; i++ )
	{
		if( i % 8 == 0 )
		{
			logging_printf( LOGGING_DEBUG, "\n");
		}
		c = buffer[i];
		logging_printf(LOGGING_DEBUG, "%02x %c\t", c, isprint(c)  ? c : '.' );
	}
	logging_printf(LOGGING_DEBUG, "\n");
	logging_printf(LOGGING_DEBUG, "-- end hex_dump\n");

	logging_prefix_enable();

	utils_unlock();
}

int check_file_security( const char *filepath )
{
	struct stat buf;

	if( ! filepath ) return 0;
	
	if( stat( filepath, &buf ) != 0 )
	{
		if( errno == ENOENT )
		{
			return 1;
		}
		logging_printf(LOGGING_ERROR, "stat() error: %s\t%s\n", errno, filepath, strerror( errno) );
		return 0;
	}

	if( ! S_ISREG( buf.st_mode ) || ( buf.st_mode & ( S_IXUSR | S_IXGRP | S_IXOTH) ) )
	{
		return 0;
	}
	
	return 1;
}

int is_yes( const char *value )
{
	if( ! value ) return 0;
	if( strcasecmp( value, "yes" ) == 0 ) return 1;
	if( strcasecmp( value, "y" ) == 0 ) return 1;
	if( strcasecmp( value, "1" ) == 0 ) return 1;

	return 0;
}

int is_no( const char *value )
{
	if( ! value ) return 0;
	if( strcasecmp( value, "no" ) == 0 ) return 1;
	if( strcasecmp( value, "n" ) == 0 ) return 1;
	if( strcasecmp( value, "0" ) == 0 ) return 1;

	return 0;
}

char *get_ip_string( struct sockaddr *sa, char *s, size_t maxlen )
{
        switch(sa->sa_family) {
        case AF_INET:
            inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr), s, maxlen);
            break;  
        case AF_INET6:
            inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr), s, maxlen);
            break;  
        default:
            strncpy(s, "Unknown AF", maxlen);
            return NULL;
        }       
        return s;
}

int get_sock_info( char *ip_address, int port, struct sockaddr *socket, socklen_t *socklen, int *family)
{
        struct addrinfo hints;
        struct addrinfo *result;
	struct addrinfo *addr;
        int val;
        char portaddr[32];
	char ip_string[100];

        if( ! ip_address ) return 1;

        memset( &hints, 0, sizeof( hints ) );
        memset( portaddr, 0, sizeof( portaddr ) );
        sprintf( portaddr, "%d", port ); 
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV | AI_NUMERICHOST | AI_CANONNAME;

        val = getaddrinfo(ip_address, portaddr, &hints, &result );
	if( val )
	{
		logging_printf(LOGGING_WARN, "get_sock_info: Invalid address: [%s]:%d\n", ip_address, port );
		return 1;
	}

	/* Display all the results */
	addr = result;
	while(addr)
	{
		logging_printf(LOGGING_DEBUG, "get_sock_info: result: ai_flags=%d, ai_family=%d, ai_sock_type=%d, ai_protocol=%d, ai_addrlen=%u, ai_canonname=[%s]\n",
			addr->ai_flags, addr->ai_family, addr->ai_socktype, addr->ai_protocol, addr->ai_addrlen, addr->ai_canonname );

		if( addr->ai_addr )
		{
			struct sockaddr_in *sock_in;

			sock_in = ( struct sockaddr_in *)addr->ai_addr;

			memset( ip_string, 0, 100 );
			get_ip_string( addr->ai_addr, ip_string, 100 );
			logging_printf(LOGGING_DEBUG, "\t\tai_addr=[ sin_family=%d, sin_port=%d, sin_addr=%s]\n",
				sock_in->sin_family, ntohs(sock_in->sin_port), ip_string );
		}
		addr = addr->ai_next;
	}

	if( socket )
	{
		memset( socket, 0, sizeof( struct sockaddr ) );
		memcpy( socket, result->ai_addr, result->ai_addrlen );
	}

	if( socklen )
	{
		*socklen = result->ai_addrlen;
	}

	if( family )
	{
		*family = result->ai_family;
	}

	freeaddrinfo( result );
	return 0;
}

int random_number( void )
{
	int ret = 0;
	
	utils_lock();
	ret = rand_r( &random_seed );
	utils_unlock();

	return ret;
}
	
long time_in_microseconds( void )
{
	struct timeval currentTime;
	gettimeofday( &currentTime, NULL );
	return ( currentTime.tv_sec * (int)1e6 + currentTime.tv_usec ) / 100 ;
}

void utils_lock( void )
{
	X_MUTEX_LOCK( &utils_thread_lock );
}

void utils_unlock( void )
{
	X_MUTEX_UNLOCK( &utils_thread_lock );
}

void utils_teardown( void )
{
	pthread_mutex_destroy( &utils_thread_lock );
}

static void utils_mem_tracking_lock( void )
{
	X_MUTEX_LOCK( &utils_diag_mem_lock );
}

static void utils_mem_tracking_unlock( void )
{
	X_MUTEX_UNLOCK( &utils_diag_mem_lock );
}

void utils_pthread_tracking_init( void )
{
	utils_pthread_filename = getenv( UTILS_PTHREAD_TRACKING_ENV );

	if( utils_pthread_filename )
	{
		utils_pthread_enabled = 1;
		utils_pthread_fp = fopen( utils_pthread_filename, "a" );
	}
}

void utils_pthread_tracking_teardown( void )
{
	if(  utils_pthread_fp )
	{
		fclose( utils_pthread_fp );
		utils_pthread_fp = NULL;
	}

	if( utils_timestamp_string )
	{
		free( utils_timestamp_string );
		utils_timestamp_string = NULL;
	}
}

void utils_mem_tracking_init( void )
{
	utils_mem_tracking_filename = getenv( UTILS_MEM_TRACKING_ENV );

	if( utils_mem_tracking_filename )
	{
		utils_mem_tracking_enabled = 1;
	}
	pthread_mutex_init( &utils_diag_mem_lock, NULL );
}

void utils_mem_tracking_teardown( void )
{
	utils_mem_tracking_lock();	
	utils_mem_tracking_enabled = 0;
	if( utils_timestamp_string )
	{
		free( utils_timestamp_string );
		utils_timestamp_string = NULL;
	}
	utils_mem_tracking_unlock();
	pthread_mutex_destroy( &utils_diag_mem_lock );
}

char *utils_timestamp( void )
{
	struct timeval tv;
	struct timezone tz;
	gettimeofday( &tv, &tz);

	if( ! utils_timestamp_string )
	{
		utils_timestamp_string = (char *)malloc( 100 );
	}

	memset( utils_timestamp_string, 0, 100 );
	sprintf( utils_timestamp_string , "%lu.%lu:%lu", tv.tv_sec, tv.tv_usec, pthread_self());
	return utils_timestamp_string;
}

void *utils_malloc( size_t size, const char *code_file_name, unsigned int line_number )
{
	void *ptr = NULL;
	ptr = malloc( size );

#ifdef _UTILS_MEMTRACKING_
	utils_mem_tracking_lock();
	if( utils_mem_tracking_enabled && utils_mem_tracking_filename)
	{
		FILE *fp = NULL;
		fp = fopen(utils_mem_tracking_filename, "a");
		if( fp )
		{
			fprintf( fp, "%s:%p:A:%zu:%s:%u\n", utils_timestamp(), ptr, size, code_file_name, line_number );
			fclose(fp);
		}
	}
	utils_mem_tracking_unlock();
#endif

	return ptr;
}

void utils_free( void *ptr, const char *code_file_name, unsigned int line_number )
{
#ifdef _UTILS_MEMTRACKING_
	utils_mem_tracking_lock();
	if( utils_mem_tracking_enabled && utils_mem_tracking_filename )
	{
		FILE *fp = NULL;
		fp = fopen(utils_mem_tracking_filename , "a");
		if( fp )
		{
			fprintf( fp, "%s:%p:F:%s:%u\n", utils_timestamp(), ptr, code_file_name, line_number );
			fclose( fp );
		}
	}
	utils_mem_tracking_unlock();
#endif

	free( ptr );
}

void *utils_realloc( void *orig_ptr, size_t size, const char *code_file_name, unsigned int line_number )
{
	void *ptr = NULL;

	ptr = realloc( orig_ptr, size );

#ifdef _UTILS_MEMTRACKING_
	utils_mem_tracking_lock();
	if( utils_mem_tracking_enabled && utils_mem_tracking_filename )
	{
		FILE *fp = NULL;
		fp = fopen( utils_mem_tracking_filename, "a");
		if( fp )
		{
			if( orig_ptr )
			{
				fprintf( fp, "%s:%p:F:%zu:%s:%u\n", utils_timestamp(),orig_ptr, size, code_file_name, line_number );
			}
			fprintf( fp, "%s:%p:A:%zu:%s:%u\n", utils_timestamp(),ptr, size, code_file_name, line_number );
			fclose(fp);
		}
	}
	utils_mem_tracking_unlock();
#endif

	return ptr;
}

char *utils_strdup(const char *s , const char *code_file_name, unsigned int line_number)
{
	char *ptr = NULL;

	if( ! s ) return NULL;

	ptr = strdup( s );

#ifdef _UTILS_MEMTRACKING_
	utils_mem_tracking_lock();
	if( utils_mem_tracking_enabled && utils_mem_tracking_filename )
	{
		FILE *fp = NULL;
		fp = fopen( utils_mem_tracking_filename, "a");
		if( fp )
		{
			fprintf( fp, "%s:%p:D:%s:%u\n", utils_timestamp(),ptr, code_file_name, line_number );
			fclose(fp);
		}
	}
	utils_mem_tracking_unlock();
#endif

	return ptr;
}

void utils_freenull( const char *description, void **ptr , const char *code_file_name, unsigned int line_number)
{
	if( ! ptr ) return;
	if( ! *ptr ) return;

#ifdef _UTILS_MEMTRACKING_
	if( utils_mem_tracking_enabled && utils_mem_tracking_filename )
	{
		FILE *fp = NULL;
		utils_mem_tracking_lock();
		fp = fopen( utils_mem_tracking_filename, "a");
		if( fp )
		{
			fprintf( fp, "%s:%p:F:%s:%u\n", utils_timestamp(),*ptr, code_file_name, line_number );
			fclose(fp);
		}
		utils_mem_tracking_unlock();
	}
#endif
	
	free( *ptr );
	*ptr = NULL;
}

void utils_pthread_mutex_lock( pthread_mutex_t *mutex , const char *code_file_name, unsigned int line_number)
{
	if( ! mutex ) return;

#ifdef _UTILS_PTHREAD_
	if( utils_pthread_fp )
	{
		fprintf( utils_pthread_fp, "%s:w:%p:%s:%u\n", utils_timestamp(), mutex, code_file_name, line_number );
	}
#endif
	pthread_mutex_lock( mutex );
#ifdef _UTILS_PTHREAD_
	if( utils_pthread_fp )
	{
		fprintf( utils_pthread_fp, "%s:l:%p:%s:%u\n", utils_timestamp(), mutex, code_file_name, line_number );
	}
#endif
}


void utils_pthread_mutex_unlock( pthread_mutex_t *mutex, const char *code_file_name, unsigned int line_number)
{
	if( ! mutex ) return;

	pthread_mutex_unlock( mutex );
#ifdef _UTILS_PTHREAD_
	if( utils_pthread_fp )
	{
		fprintf( utils_pthread_fp, "%s:u:%p:%s:%u\n", utils_timestamp(), mutex, code_file_name, line_number );
	}
#endif
}

void utils_init( void )
{
	utils_pthread_tracking_init();
        utils_mem_tracking_init();
	pthread_mutex_init( &utils_thread_lock , NULL);
	random_seed = time(NULL);
}

