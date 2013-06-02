#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include "midi_journal.h"
#include "utils.h"

journal_header_t * journal_header_create( void )
{
	journal_header_t *journal_header = NULL;

	journal_header = ( journal_header_t * ) malloc( sizeof( journal_header_t ) );
	
	if( journal_header )
	{
		memset( journal_header , 0 , sizeof( journal_header_t ) );
	}

	return journal_header;
}


void channel_header_destroy( channel_header_t **header )
{
	FREENULL( (void **)header );
}

channel_header_t * new_channel_header( void )
{
	channel_header_t *channel_header = NULL;

	channel_header = ( channel_header_t * ) malloc( sizeof( channel_header_t ) );

	if( channel_header )
	{
		memset( channel_header, 0 , sizeof( channel_header_t ) );
	}

	return channel_header;
}

void chapter_n_header_destroy( chapter_n_header_t **header )
{
	FREENULL( (void **)header);
}

chapter_n_header_t * new_chapter_n_header( void )
{
	chapter_n_header_t *chapter_n_header = NULL;

	chapter_n_header = ( chapter_n_header_t *) malloc( sizeof( chapter_n_header_t ) );

	if( chapter_n_header )
	{
		memset( chapter_n_header, 0 , sizeof( chapter_n_header_t) );
	}

	return chapter_n_header;
}

void midi_note_destroy( midi_note_t **note )
{
	FREENULL( (void **)note );
}

midi_note_t * new_midi_note( void )
{
	midi_note_t *note = NULL;
	
	note = ( midi_note_t * ) malloc( sizeof( midi_note_t ) );

	if( note )
	{
		memset( note , 0 , sizeof( midi_note_t ) );
	}

	return note;
}


chapter_n_t * new_chapter_n_journal( void )
{
	chapter_n_t *chaptern = NULL;
	unsigned int i = 0;

	chaptern = ( chapter_n_t * ) malloc( sizeof( chapter_n_t ) );

	if( chaptern )
	{
		chapter_n_header_t *header = new_chapter_n_header();

		if( ! header )
		{
			free( chaptern );
			return NULL;
		}

		chaptern->header = header;

		chaptern->num_notes = 0;
		for( i = 0 ; i < MAX_CHAPTER_N_NOTES ; i++ )
		{
			chaptern->notes[i] = NULL;
		}
	}

	return chaptern;
}

void chapter_n_destroy( chapter_n_t **chapter_n )
{
	int i;
	if( ! chapter_n ) return;
	if( ! *chapter_n ) return;

	for( i = 0 ; i < MAX_CHAPTER_N_NOTES ; i++ )
	{
		if( (*chapter_n)->notes[i] )
		{
			midi_note_destroy( &( (*chapter_n)->notes[i] ) );
		}
	}

	if( (*chapter_n)->header )
	{
		chapter_n_header_destroy( &( (*chapter_n)->header ) );
		(*chapter_n)->header = NULL;
	}

	free( *chapter_n );

	*chapter_n = NULL;

	return;
}

void channel_journal_destroy( channel_journal_t **channel )
{
	if( ! channel ) return;
	if( ! *channel ) return;

	if( (*channel)->chapter_n )
	{
		chapter_n_destroy( &( (*channel)->chapter_n ) );
	}

	if( (*channel)->header )
	{
		channel_header_destroy( &( (*channel)->header ) );
	}

	free( *channel );
	*channel = NULL;
}

channel_journal_t * new_channel_journal( void )
{
	channel_journal_t *new_channel = NULL;
	channel_header_t *new_header = NULL;
	chapter_n_t *new_chapter_n = NULL;
	
	new_channel = ( channel_journal_t * ) malloc( sizeof( channel_journal_t ) );

	if( ! new_channel ) return NULL;

	new_header = new_channel_header();

	if( ! new_header )
	{
		channel_journal_destroy( &new_channel );
		return NULL;
	}

	new_channel->header = new_header;

	new_chapter_n = new_chapter_n_journal();

	if( ! new_chapter_n )
	{
		channel_journal_destroy( &new_channel );
		return NULL;
	}

	new_channel->chapter_n = new_chapter_n;
		
	return new_channel;
}



int midi_journal_init( midi_journal_t **journal )
{
	journal_header_t *header;
	unsigned char i;

	*journal = ( midi_journal_t * ) malloc( sizeof ( midi_journal_t ) );

	if( ! journal )
	{
		return -1;
	}

	header = journal_header_create();
		
	if( ! header )
	{
		free( *journal );
		*journal = NULL;
		return -1;
	}

	(*journal)->header = header;

	for( i = 0 ; i < MAX_MIDI_CHANNELS ; i++ )
	{
		(*journal)->channels[i] = NULL;
	}

	return 0;
}

void midi_journal_destroy( midi_journal_t **journal )
{
	unsigned int i = 0;

	if( ! journal ) return;
	if( ! *journal) return;

	for( i = 0 ; i < MAX_MIDI_CHANNELS ; i++ )
	{
		if( (*journal)->channels[i] )
		{
			channel_journal_destroy( &( (*journal)->channels[i] ) );
		}
	}

	if( (*journal)->header )
	{
		free( (*journal)->header );
		(*journal)->header = NULL;
	}

	free( *journal );
	*journal = NULL;
}

void midi_journal_add_note( midi_journal_t *journal, uint32_t seq, char channel, char note, char velocity )
{
	uint16_t note_slot = 0;
	midi_note_t *new_note = NULL;

	if( ! journal ) return;

	if( channel < 1 || channel > MAX_MIDI_CHANNELS ) return;

	if( ! journal->channels[ channel - 1 ] )
	{
		channel_journal_t *channel_journal = new_channel_journal();

		if( ! channel_journal ) return;
		journal->channels[ channel - 1 ] = channel_journal;

	}

	if( journal->channels[ channel - 1 ]->chapter_n->num_notes == MAX_CHAPTER_N_NOTES ) return;

	new_note = new_midi_note();
	if(! new_note ) return;

	new_note->num = note;
	new_note->velocity = velocity;

	note_slot = journal->channels[ channel - 1]->chapter_n->num_notes++;

	journal->channels[ channel - 1]->chapter_n->notes[note_slot] = new_note;

	journal->channels[ channel - 1]->header->bitfield |= CHAPTER_N;

	if( journal->channels[ channel - 1 ]->header->chan != channel )
	{
		journal->channels[ channel - 1]->header->chan = channel;
		journal->header->totchan +=1;
	}

	journal->header->seq = seq;
	fprintf(stderr, "Setting journal header seq to %u\n", journal->header->seq);
}

void midi_note_dump( midi_note_t *note )
{
	if( ! note ) return;

	fprintf(stderr, "NOTE: S=%d num=%u Y=%d velocity=%u\n", note->S, note->num, note->Y, note->velocity);
}

void chapter_n_header_dump( chapter_n_header_t *header )
{
	if( ! header ) return;

	fprintf(stderr, "Chapter N(header): B=%d len=%u low=%u high=%u\n", header->B, header->len, header->low, header->high);
}

void chapter_n_dump( chapter_n_t *chaptern )
{
	uint16_t i = 0;

	if( ! chaptern ) return;

	fprintf(stderr, "Chapter N\n");

	chapter_n_header_dump( chaptern->header );

	for( i = 0 ; i < chaptern->num_notes ; i++ )
	{
		midi_note_dump( chaptern->notes[i] );
	}
	
	fprintf(stderr, "END\n");
}

void channel_header_dump( channel_header_t *header )
{
	if( ! header ) return;

	fprintf(stderr, "Channel #%d (Header: S=%d H=%d len=%u bitfield=%02x)\n", header->chan, header->S, header->H, header->len, header->bitfield);
}

void channel_journal_dump( channel_journal_t *channel )
{
	if( ! channel ) return;

	channel_header_dump( channel->header );

	if( channel->header->bitfield && CHAPTER_N )
	{
		chapter_n_dump( channel->chapter_n );
	}
}

void journal_header_dump( journal_header_t *header )
{
	if( ! header ) return;

	fprintf(stderr, "Journal (Header: bitfield=%02x totchan=%d seq=%04x)\n", header->bitfield, header->totchan, header->seq);
}

void journal_dump( midi_journal_t *journal )
{
	unsigned int i = 0;
	if( ! journal ) return;

	journal_header_dump( journal->header );

	for( i = 0 ; i < MAX_MIDI_CHANNELS ; i++ )
	{
		channel_journal_dump( journal->channels[i] );
	}
}
