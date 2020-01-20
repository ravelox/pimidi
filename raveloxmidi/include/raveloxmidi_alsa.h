#ifdef HAVE_ALSA

#ifndef _RAVELOXMIDI_ALSA_H
#define _RAVELOXMIDI_ALSA_H

#include <asoundlib.h>

void raveloxmidi_alsa_list_rawmidi_devices( void );
void raveloxmidi_alsa_init( char *input_name, char *output_name , size_t buffer_size);
void raveloxmidi_alsa_handle_destroy( void **rawmidi );
void raveloxmidi_alsa_teardown( void );

void raveloxmidi_alsa_dump_rawmidi( void *data );

int raveloxmidi_alsa_out_available( void );
int raveloxmidi_alsa_in_available( void );

int raveloxmidi_alsa_write( unsigned char *buffer, size_t buffer_size );
int raveloxmidi_alsa_read( snd_rawmidi_t *handle, unsigned char *buffer, size_t read_size); 
int raveloxmidi_alsa_poll( int timeout );
void raveloxmidi_alsa_set_poll_fds( snd_rawmidi_t *handle );

int raveloxmidi_alsa_loop( void );
void raveloxmidi_wait_for_alsa( void );

#define RAVELOXMIDI_ALSA_INPUT	-2
#define RAVELOXMIDI_ALSA_DEFAULT_BUFFER	4096
#define RAVELOXMIDI_ALSA_MAX_BUFFER	1048576

#endif

#endif
