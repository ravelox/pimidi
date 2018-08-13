#ifdef HAVE_ALSA

#ifndef _RAVELOXMIDI_ALSA_H
#define _RAVELOXMIDI_ALSA_H

#include <asoundlib.h>

void raveloxmidi_alsa_list_rawmidi_devices( void );
int raveloxmidi_alsa_init( unsigned char *device_name );
void raveloxmidi_alsa_handle_destroy( snd_rawmidi_t *rawmidi );
void raveloxmidi_alsa_destroy( void );

void raveloxmidi_alsa_dump_rawmidi( snd_rawmidi_t *rawmidi );

int raveloxmidi_alsa_output_available( void );
int raveloxmidi_alsa_output( unsigned char *buffer, size_t buffer_size );

#endif

#endif
