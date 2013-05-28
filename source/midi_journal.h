#ifndef MIDI_JOURNAL_H
#define MIDI_JOURNAL_H

typedef struct recovery_journal_header_t {
	unsigned	bitfield:4; // SYAH
	unsigned 	totchan:4;
	uint16_t	seq;
} recovery_journal_header_t;

typedef struct channel_journal_header_t {
	unsigned	S:1;
	unsigned	chan:4;
	unsigned	H:1;
	unsigned	len:10;
	uint8_t		bitfield; // PCMWNETA
} channel_journal_header_t;

typedef struct chapter_n_journal_header_t {
	unsigned	B:1;
	unsigned	len:7;
	unsigned	low:4;
	unsigned	high:4;
} chapter_n_journal_header_t;

typedef struct midi_note_t {
	unsigned	S:1;
	unsigned	num:7;
	unsigned	Y:1;
	unsigned	velocity:7;
} midi_note_t;

typedef struct chapter_n_journal_t {
	chapter_n_journal_header_t *header;
	midi_note_t **notes;
} chapter_n_journal_t;

typedef struct channel_journal_t {
	channel_journal_header_t *header;
	chapter_n_journal_t *chapter_n;
} channel_journal_t;

typedef struct recovery_journal_t {
	recovery_journal_header_t *header;
	channel_journal_t channels[16];
} recovery_journal_t;

midi_note_t * new_midi_note( void );
recovery_journal_t *journal_init( void );
void journal_destroy( recovery_journal_t * );

#endif
