#ifndef MIDI_PAYLOAD_H
#define MIDI_PAYLOAD_H

typedef struct midi_payload_header_t {
	unsigned char	B;
	unsigned char	J;
	unsigned char	Z;
	unsigned char	P;
	uint16_t	len;
} midi_payload_header_t;

#define PAYLOAD_HEADER_B	0x80
#define PAYLOAD_HEADER_J	0x40
#define PAYLOAD_HEADER_Z	0x20
#define PAYLOAD_HEADER_P	0x10
#define PAYLOAD_HEADER_LEN	0x0f

typedef struct midi_payload_t {
	midi_payload_header_t *header;
	char		*buffer;
} midi_payload_t;

midi_payload_t *midi_payload_create( void );
void midi_payload_destroy( midi_payload_t **payload );
void payload_reset( midi_payload_t **payload );
void payload_toggle_b( midi_payload_t *payload );
void payload_toggle_j( midi_payload_t *payload );
void payload_toggle_z( midi_payload_t *payload );
void payload_toggle_p( midi_payload_t *payload );
void payload_set_buffer( midi_payload_t *payload, char *buffer , uint16_t buffer_size);
void payload_pack( midi_payload_t *payload, unsigned char **buffer, size_t *buffer_size);

#endif
