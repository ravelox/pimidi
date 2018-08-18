#ifdef HAVE_ALSA

#ifndef _RAVELOXMIDI_ALSA_H
#define _RAVELOXMIDI_ALSA_H

#include <asoundlib.h>

void raveloxmidi_alsa_list_rawmidi_devices( void );
void raveloxmidi_alsa_init( char *input_name, char *output_name , size_t buffer_size);
void raveloxmidi_alsa_handle_destroy( snd_rawmidi_t *rawmidi );
void raveloxmidi_alsa_destroy( void );

void raveloxmidi_alsa_dump_rawmidi( snd_rawmidi_t *rawmidi );

int raveloxmidi_alsa_output_available( void );
int raveloxmidi_alsa_input_available( void );

int raveloxmidi_alsa_write( unsigned char *buffer, size_t buffer_size );
int raveloxmidi_alsa_read( unsigned char *buffer, size_t read_size); 

#define RAVELOXMIDI_ALSA_INPUT	-2
#define RAVELOXMIDI_ALSA_DEFAULT_BUFFER	4096
#define RAVELOXMIDI_ALSA_MAX_BUFFER	1048576

#endif

#endif
