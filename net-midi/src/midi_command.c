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
