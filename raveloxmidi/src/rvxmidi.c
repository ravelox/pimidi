#include <stdio.h>
#include <stdlib.h>

#include "utils.h"
#include "raveloxmidi_config.h"
#include "logging.h"

#include "rvxmidi/rvxmidi.h"

int rvxmidi_init( int argc, char *argv[] )
{
	int ret = 0;

	utils_init();

	ret = config_init(argc, argv);

	logging_init();

	return ret;
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
