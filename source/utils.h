#ifndef UTILS_H
#define UTILS_H

uint64_t ntohll(const uint64_t value);
uint64_t htonll(const uint64_t value);
void get_uint16( void *dest, unsigned char **src, size_t *len );
void put_uint16( unsigned char **dest, uint16_t src, size_t *len );
void put_uint32( unsigned char **dest, uint32_t src, size_t *len );
void put_uint64( unsigned char **dest, uint64_t src, size_t *len );
void get_uint32( void *dest, unsigned char **src, size_t *len );
void get_uint64( void *dest, unsigned char **src, size_t *len );
void hex_dump( unsigned char *buffer, size_t len );

void FREENULL( void **ptr );

#endif
