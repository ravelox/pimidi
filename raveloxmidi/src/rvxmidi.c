#include <stdio.h>
#include <stdlib.h>

#include "utils.h"
#include "raveloxmidi_config.h"
#include "logging.h"

#include "rvxmidi/rvxmidi.h"

void rvxmidi_init( void )
{
	utils_init();
	config_init();
	logging_init();
}

void rvxmidi_config_load( char *filename )
{
	if( ! filename ) return;
	config_load_file( filename );
}

void rvxmidi_teardown( void )
{
	config_teardown();

        logging_teardown();

        utils_teardown();
        utils_mem_tracking_teardown();
        utils_pthread_tracking_teardown();
}

void rvxmidi_register_midi_note(void)
{
}
