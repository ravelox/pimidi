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

void journal_header_destroy( journal_header_t **header )
{
	FREENULL( (void **)header );
}


void channel_header_destroy( channel_header_t **header )
{
	FREENULL( (void **)header);
}

channel_header_t * channel_header_create( void )
{
	channel_header_t *header = NULL;

	header = ( channel_header_t * ) malloc( sizeof( channel_header_t ) );

	if( header )
	{
		memset( header, 0 , sizeof( channel_header_t ) );
	}

	return header;
}

void chaptern_header_destroy( chaptern_header_t **header )
{
	FREENULL( (void **)header);
}

chaptern_header_t * chaptern_header_create( void )
{
	chaptern_header_t *header = NULL;

	header = ( chaptern_header_t *) malloc( sizeof( chaptern_header_t ) );

	if( header )
	{
		memset( header, 0 , sizeof( chaptern_header_t) );
		header->low = 0xf;
	}

	return header;
}

void midi_note_destroy( midi_note_t **note )
{
	FREENULL( (void **)note );
}

midi_note_t * midi_note_create( void )
{
	midi_note_t *note = NULL;
	
	note = ( midi_note_t * ) malloc( sizeof( midi_note_t ) );

	if( note )
	{
		memset( note , 0 , sizeof( midi_note_t ) );
	}

	return note;
}


chaptern_t * chaptern_create( void )
{
	chaptern_t *chaptern = NULL;
	unsigned int i = 0;

	chaptern = ( chaptern_t * ) malloc( sizeof( chaptern_t ) );

	if( chaptern )
	{
		chaptern_header_t *header = chaptern_header_create();

		if( ! header )
		{
			free( chaptern );
			return NULL;
		}

		chaptern->header = header;

		chaptern->num_notes = 0;
		for( i = 0 ; i < MAX_CHAPTERN_NOTES ; i++ )
		{
			chaptern->notes[i] = NULL;
		}
		
		chaptern->offbits = ( char *)malloc( MAX_OFFBITS );
		if (! chaptern->offbits )
		{
			chaptern_destroy( &chaptern );
			return NULL;
		}
			
		memset( chaptern->offbits, 0, MAX_OFFBITS );
	}

	return chaptern;
}

void chaptern_destroy( chaptern_t **chaptern )
{
	int i;
	if( ! chaptern ) return;
	if( ! *chaptern ) return;

	for( i = 0 ; i < MAX_CHAPTERN_NOTES ; i++ )
	{
		if( (*chaptern)->notes[i] )
		{
			midi_note_destroy( &( (*chaptern)->notes[i] ) );
		}
	}

	if( (*chaptern)->offbits )
	{
		free( (*chaptern)->offbits );
		(*chaptern)->offbits = NULL;
	}

	if( (*chaptern)->header )
	{
		chaptern_header_destroy( &( (*chaptern)->header ) );
		(*chaptern)->header = NULL;
	}

	free( *chaptern );

	*chaptern = NULL;

	return;
}

void channel_destroy( channel_t **channel )
{
	if( ! channel ) return;
	if( ! *channel ) return;

	if( (*channel)->chapter_n )
	{
		chaptern_destroy( &( (*channel)->chapter_n ) );
	}

	if( (*channel)->header )
	{
		channel_header_destroy( &( (*channel)->header ) );
	}

	free( *channel );
	*channel = NULL;
}

channel_t * channel_create( void )
{
	channel_t *new_channel = NULL;
	channel_header_t *new_header = NULL;
	chaptern_t *new_chapter_n = NULL;
	
	new_channel = ( channel_t * ) malloc( sizeof( channel_t ) );

	if( ! new_channel ) return NULL;

	new_header = channel_header_create();

	if( ! new_header )
	{
		channel_destroy( &new_channel );
		return NULL;
	}

	new_channel->header = new_header;

	new_chapter_n = chaptern_create();

	if( ! new_chapter_n )
	{
		channel_destroy( &new_channel );
		return NULL;
	}

	new_channel->chapter_n = new_chapter_n;
		
	return new_channel;
}

int journal_init( journal_t **journal )
{
	journal_header_t *header;
	unsigned char i;

	*journal = ( journal_t * ) malloc( sizeof ( journal_t ) );

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

void journal_destroy( journal_t **journal )
{
	unsigned int i = 0;

	if( ! journal ) return;
	if( ! *journal) return;

	for( i = 0 ; i < MAX_MIDI_CHANNELS ; i++ )
	{
		if( (*journal)->channels[i] )
		{
			channel_destroy( &( (*journal)->channels[i] ) );
		}
	}

	if( (*journal)->header )
	{
		journal_header_destroy( &( (*journal)->header ) );
	}

	free( *journal );
	*journal = NULL;
}

void midi_journal_add_note( journal_t *journal, uint32_t seq, char channel, char note, char velocity )
{
	uint16_t note_slot = 0;
	midi_note_t *new_note = NULL;

	if( ! journal ) return;

	if( channel < 1 || channel > MAX_MIDI_CHANNELS ) return;

	if( ! journal->channels[ channel - 1 ] )
	{
		channel_t *channel_journal = channel_create();

		if( ! channel_journal ) return;
		journal->channels[ channel - 1 ] = channel_journal;

	}

	// Store velocity 0 as NoteOff
	if( velocity == 0 )
	{
		uint8_t offset, shift;
		// Which element
		offset = note / 8;
		shift = (note - ( offset * 8 )) - 1;

		// Set low and high values;
		if( offset > journal->channels[channel - 1]->chapter_n->header->high )
		{
			journal->channels[channel - 1]->chapter_n->header->high = offset;
		}

		if( offset < journal->channels[channel - 1]->chapter_n->header->low )
		{
			journal->channels[channel - 1]->chapter_n->header->low = offset;
		}

		journal->header->seq = seq;
		journal->channels[channel - 1]->chapter_n->offbits[offset] |=  ( 1 << shift );

		return;
	}

	if( journal->channels[ channel - 1 ]->chapter_n->num_notes == MAX_CHAPTERN_NOTES ) return;

	new_note = midi_note_create();
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

void chapter_n_header_dump( chaptern_header_t *header )
{
	if( ! header ) return;

	fprintf(stderr, "Chapter N(header): B=%d len=%u low=%u high=%u\n", header->B, header->len, header->low, header->high);
}

void chapter_n_dump( chaptern_t *chaptern )
{
	uint16_t i = 0;

	if( ! chaptern ) return;

	chapter_n_header_dump( chaptern->header );

	for( i = 0 ; i < chaptern->num_notes ; i++ )
	{
		midi_note_dump( chaptern->notes[i] );
	}
	
	for( i = chaptern->header->low; i <= chaptern->header->high ; i++ )
	{
		fprintf(stderr, "Offbits[%d]=%02x\n", i, chaptern->offbits[i]);
	}

	fprintf(stderr, "End Chapter N\n");
}

void channel_header_dump( channel_header_t *header )
{
	if( ! header ) return;

	fprintf(stderr, "Channel #%d (Header: S=%d H=%d len=%u bitfield=%02x)\n", header->chan, header->S, header->H, header->len, header->bitfield);
}

void channel_journal_dump( channel_t *channel )
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

void journal_dump( journal_t *journal )
{
	unsigned int i = 0;
	if( ! journal ) return;

	journal_header_dump( journal->header );

	for( i = 0 ; i < MAX_MIDI_CHANNELS ; i++ )
	{
		channel_journal_dump( journal->channels[i] );
	}
}

uint16_t calc_channel_size( channel_t *channel )
{

	uint16_t size = 0;

	if( ! channel ) return 0;

	size += sizeof( channel_header_t );
	
	size += sizeof( chaptern_header_t );

	return size;
}

void journal_buffer_create( journal_t *journal, char **buffer, uint32_t *size )
{
	int i;
	char *p;
	uint16_t channel_size;

	*buffer = NULL;
	*size = 0;

	if( ! journal ) return;

	fprintf(stderr, "\n\nJournal Buffer\n");
	for( i = 0 ; i < MAX_MIDI_CHANNELS; i++ )
	{
		if( ! journal->channels[i] ) continue;

		if( ! *buffer )
		{
			*buffer = ( char * ) malloc( sizeof( journal_header_t ) );
			memcpy( *buffer, journal->header, sizeof( journal_header_t ) );
			*size += sizeof( journal_header_t );
		}

		*buffer = (char *)realloc( *buffer, *size + sizeof( channel_header_t ) );
		*p = (*buffer) + *size;

		channel_size = calc_channel_size( journal->channels[i] );

		//memcpy( *buffer, journal->channels[i]->header, sizeof( channel_header_t ) );
	}
}
