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

#ifndef UTILS_H
#define UTILS_H

#include <netinet/in.h>
#include <stdint.h>
#include <unistd.h>

uint64_t ntohll(const uint64_t value);
uint64_t htonll(const uint64_t value);
void get_uint16( void *dest, unsigned char **src, size_t *len );
void put_uint16( unsigned char **dest, uint16_t src, size_t *len );
void put_uint32( unsigned char **dest, uint32_t src, size_t *len );
void put_uint64( unsigned char **dest, uint64_t src, size_t *len );
void get_uint32( void *dest, unsigned char **src, size_t *len );
void get_uint64( void *dest, unsigned char **src, size_t *len );
void hex_dump( unsigned char *buffer, size_t len );
int check_file_security( const char *filepath );
int is_yes( const char *value );
int is_no( const char *value );

char *get_ip_string( struct sockaddr *sa, char *s, size_t maxlen );
int get_sock_info( char *ip_address, int port, struct sockaddr *socket, socklen_t *socklen, int *family);

int random_number( void );
long time_in_microseconds( void );

void utils_lock( void );
void utils_unlock( void );

void utils_init( void );
void utils_teardown( void );

// Memory tracking utilities
void utils_mem_tracking_init( void );
void utils_mem_tracking_teardown( void );

void *utils_malloc( size_t size, const char *code_file_name, unsigned int line_number );
void utils_free( void *ptr , const char *code_file_name, unsigned int line_number);
void *utils_realloc( void *orig_ptr, size_t new_size, const char *code_file_name, unsigned int line_number );
char *utils_strdup( const char *s , const char *code_file_name, unsigned int line_number);
void utils_freenull( const char *description, void **ptr, const char *code_file_name, unsigned int line_number );

#define X_MALLOC(a)	utils_malloc( a, __FILE__, __LINE__ )
#define X_FREE(a)	utils_free( a, __FILE__, __LINE__ )
#define X_REALLOC(a,b)	utils_realloc( a,b, __FILE__, __LINE__ )
#define X_STRDUP(a)	utils_strdup( a, __FILE__, __LINE__ )
#define X_FREENULL(a,b) utils_freenull( a, b, __FILE__, __LINE__ )

// Mutex tracking utilities
void utils_pthread_tracking_init( void );
void utils_pthread_tracking_teardown( void );
void utils_pthread_mutex_lock( pthread_mutex_t *mutex, const char *code_file_name, unsigned int line_number);
void utils_pthread_mutex_unlock( pthread_mutex_t *mutex, const char *code_file_name, unsigned int line_number);

#define X_MUTEX_LOCK(a)	utils_pthread_mutex_lock( a, __FILE__, __LINE__ )
#define X_MUTEX_UNLOCK(a)	utils_pthread_mutex_unlock( a, __FILE__, __LINE__ )

// Generic macros
#define MAX(a,b) ( (a) > (b) ? (a) : (b) )
#define MIN(a,b) ( (a) < (b) ? (a) : (b) )

#endif
