#ifndef MIDI_RECV_H
#define MIDI_RECV_H

typedef struct midi_note_packet_t {
	char	signature;
	char	channel;
	char	note;
	char	velocity;
} midi_note_packet_t;

int midi_note_unpack( midi_note_packet_t **midi_note, char *packet, size_t packet_len );

#endif
