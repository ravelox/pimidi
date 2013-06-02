#ifndef MIDI_JOURNAL_H
#define MIDI_JOURNAL_H

typedef struct journal_header_t {
	unsigned	bitfield:4; // SYAH
	unsigned 	totchan:4;
	uint16_t	seq;
} journal_header_t;

typedef struct channel_header_t {
	unsigned	S:1;
	unsigned	chan:4;
	unsigned	H:1;
	unsigned	len:10;
	uint8_t		bitfield; // PCMWNETA
} channel_header_t;

#define CHAPTER_P	0x80
#define CHAPTER_C	0x40
#define CHAPTER_M	0x20
#define CHAPTER_W	0x10
#define CHAPTER_N	0x08
#define CHAPTER_E	0x04
#define CHAPTER_T	0x02
#define CHAPTER_A	0x01

typedef struct chapter_n_header_t {
	unsigned	B:1;
	unsigned	len:7;
	unsigned	low:4;
	unsigned	high:4;
} chapter_n_header_t;

typedef struct midi_note_t {
	unsigned	S:1;
	unsigned	num:7;
	unsigned	Y:1;
	unsigned	velocity:7;
} midi_note_t;

#define MAX_CHAPTER_N_NOTES	127

typedef struct chapter_n_t {
	chapter_n_header_t *header;
	uint16_t	num_notes;
	midi_note_t *notes[MAX_CHAPTER_N_NOTES];
	uint8_t	offbits[16];
} chapter_n_t;

typedef struct channel_journal_t {
	channel_header_t *header;
	chapter_n_t *chapter_n;
} channel_journal_t;

#define MAX_MIDI_CHANNELS	16

typedef struct midi_journal_t {
	journal_header_t *header;
	channel_journal_t *channels[MAX_MIDI_CHANNELS];
} midi_journal_t;


journal_header_t * journal_header_create( void );
void channel_header_destroy( channel_header_t **header );
channel_header_t * new_channel_header( void );
void chapter_n_header_destroy( chapter_n_header_t **header );
chapter_n_header_t * new_chapter_n_header( void );
void midi_note_destroy( midi_note_t **note );
midi_note_t * new_midi_note( void );
chapter_n_t * new_chapter_n_journal( void );
void chapter_n_destroy( chapter_n_t **chapter_n );
void channel_journal_destroy( channel_journal_t **channel );
channel_journal_t * new_channel_journal( void );
int midi_journal_init( midi_journal_t **journal );
void midi_journal_destroy( midi_journal_t **journal );
void midi_journal_add_note( midi_journal_t *journal, uint32_t seq, char channel, char note, char velocity );
void channel_header_dump( channel_header_t *header );
void channel_journal_dump( channel_journal_t *channel );
void journal_header_dump( journal_header_t *header );
void journal_dump( midi_journal_t *journal );

#endif
