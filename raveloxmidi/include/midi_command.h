#ifndef MIDI_COMMAND_H
#define MIDI_COMMAND_H

typedef struct midi_command_t {
	uint8_t	command;
	uint8_t	note;
	uint8_t velocity;
} midi_command_t;

#define MIDI_COMMAND_NOTE_ON	0x90
#define MIDI_COMMAND_NOTE_OFF	0x80

#endif
