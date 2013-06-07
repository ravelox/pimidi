#ifndef MIDI_PAYLOAD_H
#define MIDI_PAYLOAD_H

typedef struct midi_payload_header_t {
	unsigned char	B:1;
	unsigned char	J:1;
	unsigned char	Z:1;
	unsigned char	P:1;
	unsigned char	len:4;
} midi_payload_header_t;

typedef struct midi_payload_t {
	midi_payload_header_t	*header;
	char			*payload;
} midi_payload_t;

#endif
