#ifndef MIDI_PAYLOAD_H
#define MIDI_PAYLOAD_H

typedef struct midi_payload_header_t {
	unsigned char	B:1;
	unsigned char	J:1;
	unsigned char	Z:1;
	unsigned char	P:1;
	unsigned char	len:4;
} midi_payload_header_t;

#define PAYLOAD_HEADER_B	0x80
#define PAYLOAD_HEADER_J	0x40
#define PAYLOAD_HEADER_Z	0x20
#define PAYLOAD_HEADER_P	0x10
#define PAYLOAD_HEADER_LEN	0x0f

typedef struct midi_payload_t {
	unsigned char	header;
	char		*buffer;
} midi_payload_t;

#endif
