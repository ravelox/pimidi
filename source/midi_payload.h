#ifndef MIDI_PAYLOAD_H
#define MIDI_PAYLOAD_H

typedef struct midi_payload_header_t {
	unsigned	B:1;
	unsigned	J:1;
	unsigned	Z:1;
	unsigned	P:1;
	unsigned	len:4;
} midi_payload_header_t;

typedef struct midi_payload_t {
	midi_payload_header_t	*header;
	char			*payload;
} midi_payload_t;

#endif
