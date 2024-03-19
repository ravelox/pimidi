#include <stdio.h>
#include <stdlib.h>

#ifndef _RVXMIDI_H
#define _RVXMIDI_H

void rvxmidi_init(void);
void rvxmidi_teardown(void);

void rvxmidi_logging_init(void);

void rvxmidi_config_load( char *filename );

int rvxmidi_service_start( void );
void rvxmidi_service_stop( void );

#endif
