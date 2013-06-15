#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include <arpa/inet.h>

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

	for( i = 0 ; i < len ; i++ )
	{
		if( i % 8 == 0 )
		{
			printf("\n");
		}
		c = buffer[i];
		printf("%02x %c\t", c, isprint(c)  ? c : '.' );
	}
	printf("\n");
}

void FREENULL( void **ptr )
{
	if( ! ptr ) return;
	if( ! *ptr ) return;

	free( *ptr );
	*ptr = NULL;
}
