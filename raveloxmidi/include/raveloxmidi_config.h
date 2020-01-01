#ifndef _RAVELOXMIDI_CONFIG_H
#define _RAVELOXMIDI_CONFIG_H

#include "kv_table.h"

void config_init( int argc, char *argv[] );
void config_teardown( void );

char *config_string_get( char *key );
int config_int_get( char *key );
long config_long_get( char *key );
int config_is_set( char *key );

void config_add_item(char *key, char *value);
void config_dump( void );

void config_usage( void );

typedef struct raveloxmidi_config_iter_t {
	char *prefix;
	int index;
} raveloxmidi_config_iter_t;

raveloxmidi_config_iter_t *config_iter_create( char *prefix );
void config_iter_destroy( raveloxmidi_config_iter_t **iter );
void config_iter_reset( raveloxmidi_config_iter_t *iter );
void config_iter_next( raveloxmidi_config_iter_t *iter  );
char *config_iter_string_get( raveloxmidi_config_iter_t *iter );
int config_iter_int_get( raveloxmidi_config_iter_t *iter );
long config_iter_long_get( raveloxmidi_config_iter_t *iter );
int config_iter_is_set( raveloxmidi_config_iter_t *iter );

#define MAX_CONFIG_LINE	1024

#define MAX_ITER_INDEX	10000

#endif
