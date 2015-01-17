#ifndef _RAVELOXMIDI_CONFIG_H
#define _RAVELOXMIDI_CONFIG_H

typedef struct raveloxmidi_config_t
{
	char *key;
	char *value;
} raveloxmidi_config_t;

void config_init( int argc, char *argv[] );
void config_destroy( void );

char *config_get( char *key );
void config_add_item(char *key, char *value);
void config_dump( void );

void config_usage( void );

#define MAX_CONFIG_LINE	1024

#endif
