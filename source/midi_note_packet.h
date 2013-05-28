#ifndef MIDI_NOTE_PACKET_H
#define MIDI_NOTE_PACKET_H

typedef struct midi_note_packet_t {
	unsigned char	signature;
	char	channel;
	char	note;
	char	velocity;
} midi_note_packet_t;

int midi_note_packet_unpack( midi_note_packet_t **midi_note, unsigned char *packet, size_t packet_len );
void midi_note_packet_destroy( midi_note_packet_t **note_packet );
void midi_note_packet_dump( midi_note_packet_t *note_packet );

#endif
