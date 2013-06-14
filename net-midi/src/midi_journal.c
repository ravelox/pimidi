#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include "midi_journal.h"
#include "utils.h"

void journal_header_pack( journal_header_t *header , char **packed , size_t *size )
{
	unsigned char *p = NULL;

	*packed = NULL;
	*size = 0;

	fprintf( stderr, ">journal_header_pack\n");
	if( ! header ) return;

	*packed = ( char *)malloc( JOURNAL_HEADER_PACKED_SIZE );

	if( ! packed ) return;

	p = *packed;

	*p |= ( ( header->bitfield & 0x0f ) << 4 );
	*p |= ( ( header->totchan == 0 ? 0 : header->totchan - 1 ) & 0x0f ) ;

	p += sizeof( char );
	*size += sizeof( char );

	put_uint16( &p, header->seq, size );
}

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


void channel_header_pack( channel_header_t *header , char **packed , size_t *size )
{
	unsigned char *p = NULL;
	uint16_t temp_header = 0;

	*packed = NULL;
	*size = 0;

	fprintf( stderr, ">channel_header_pack\n");
	if( ! header ) return;

	*packed = ( char *)malloc( CHANNEL_HEADER_PACKED_SIZE );

	if( ! packed ) return;

	p = *packed;

	fprintf(stderr, "Channel Header -> S    = %u\n", header->S );
	fprintf(stderr, "Channel Header -> chan = 0x%02x\n", header->chan );
	fprintf(stderr, "Channel Header -> H    = %u\n", header->H );
	fprintf(stderr, "Channel Header -> len  = 0x%04x\n", header->len );

	temp_header |= ( ( header->S & 0x01 ) << 15 );
	temp_header |= ( ( ( header->chan == 0 ? 0 : header->chan - 1 ) & 0x0f ) << 11 );
	temp_header |= ( ( header->H & 0x01 ) << 10 );
	temp_header |= ( ( header->len & 0x03ff ) );

	temp_header = htons( temp_header );
	memcpy( p , &temp_header, sizeof( uint16_t ) );
	p += sizeof( uint16_t );
	*size += sizeof( uint16_t );

	*p = header->bitfield;
	*size += sizeof( header->bitfield );
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

void chaptern_header_pack( chaptern_header_t *header , char **packed , size_t *size )
{
	unsigned char *p = NULL;

	*packed = NULL;
	*size = 0;

	fprintf(stderr, ">chaptern_header_pack\n");
	if( ! header ) return;

	*packed = ( char *)malloc( CHAPTERN_HEADER_PACKED_SIZE );

	if( ! packed ) return;

	p = *packed;

	*p |= ( ( header->B & 0x01 ) << 7 );
	*p |= ( header->len & 0x7f ) ;

	p += sizeof( char );
	*size += sizeof( char );

	*p |= ( ( header->low & 0x0f ) << 4 );
	*p |= ( header->high & 0x0f );

	*size += sizeof( char );
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
		header->low = 0x0f;
		header->high = 0x01;
	}

	return header;
}

void midi_note_pack( midi_note_t *note , char **packed , size_t *size )
{
	unsigned char *p = NULL;

	*packed = NULL;
	*size = 0;

	fprintf(stderr, ">midi_note_pack\n");
	if( ! note ) return;

	*packed = ( char *)malloc( MIDI_NOTE_PACKED_SIZE );

	if( ! packed ) return;

	p = *packed;

	*p |= ( ( note->S & 0x01 ) << 7 );
	*p |= ( note->num & 0x7f ) ;

	p += sizeof( char );
	*size += sizeof( char );

	*p |= ( ( note->Y & 0x01 ) << 7 );
	*p |= ( note->velocity & 0x7f );

	*size += sizeof( char );
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


void chaptern_pack( chaptern_t *chaptern, char **packed, size_t *size )
{
	char *packed_header = NULL;
	char *packed_note = NULL;
	char *note_buffer = NULL;
	char *offbits_buffer = NULL;
	char *p = NULL;
	int i = 0;
	size_t header_size, note_size, note_buffer_size;
	int offbits_size;

	*packed = NULL;
	*size = 0;

	fprintf(stderr, ">chaptern_pack\n");

	header_size = note_size = note_buffer_size = 0;

	if( ! chaptern ) return;

	chaptern->header->len = chaptern->num_notes;

	chaptern_header_pack( chaptern->header, &packed_header, &header_size) ;

	fprintf(stderr, "Chapter N Header\n");
	hex_dump( packed_header, header_size );

	if( packed_header )
	{
		*size += header_size;
	}

	if( chaptern->num_notes > 0 )
	{
		note_buffer_size = MIDI_NOTE_PACKED_SIZE * chaptern->num_notes;
		note_buffer = ( char * ) malloc( note_buffer_size );
		if( note_buffer ) 
		{
			p = note_buffer;

			for( i = 0 ; i < chaptern->num_notes ; i++ )
			{
				midi_note_pack( chaptern->notes[i], &packed_note, &note_size );
				fprintf(stderr, "Midi Note\n");
				hex_dump( packed_note, note_size );
				memcpy( p, packed_note, note_size );
				p += note_size;
				*size += note_size;
				free( packed_note );
			}
		}
	}

	offbits_size = ( chaptern->header->high - chaptern->header->low ) + 1;
	if( offbits_size > 0 )
	{
		offbits_buffer = ( char * )malloc( offbits_size );
		p = chaptern->offbits + ( offbits_size - 1 );
		memcpy( offbits_buffer, p, offbits_size );
		*size += offbits_size;
	}

	// Now pack it all together

	*packed = ( char * ) malloc( *size );

	if( ! packed ) goto chaptern_pack_cleanup;

	p = *packed;

	memcpy( p , packed_header, header_size );
	p += header_size;

	memcpy( p, note_buffer, note_buffer_size );
	p+= note_buffer_size;

	if( offbits_buffer )
	{
		memcpy( p, offbits_buffer, offbits_size );
	}

chaptern_pack_cleanup:
	FREENULL( (void **)&packed_header );
	FREENULL( (void **)&note_buffer );
	FREENULL( (void **)&offbits_buffer );
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

void channel_pack( channel_t *channel, char **packed, size_t *size )
{
	char *packed_channel_header = NULL;
	char *packed_chaptern = NULL;
	
	size_t packed_channel_header_size = 0;
	size_t packed_chaptern_size = 0;

	char *p = NULL;
	int i = 0;

	*packed = NULL;
	*size = 0;


	fprintf(stderr, ">channel_pack\n");
	if( ! channel ) return;


	chaptern_pack( channel->chaptern, &packed_chaptern, &packed_chaptern_size );
	channel->header->len = packed_chaptern_size + CHANNEL_HEADER_PACKED_SIZE;
	fprintf(stderr, "Chapter N (%u)\n", channel->header->len);
	hex_dump( packed_chaptern, packed_chaptern_size );

	channel_header_pack( channel->header, &packed_channel_header, &packed_channel_header_size );
	fprintf(stderr, "Channel Header\n");
	hex_dump( packed_channel_header , packed_channel_header_size );


	*packed = ( char * ) malloc( packed_channel_header_size + packed_chaptern_size );

	if( ! packed ) goto channel_pack_cleanup;

	p = *packed;

	memcpy( p, packed_channel_header, packed_channel_header_size );
	*size += packed_channel_header_size;
	p += packed_channel_header_size;
	
	memcpy( p, packed_chaptern, packed_chaptern_size );
	*size += packed_chaptern_size;

channel_pack_cleanup:
	FREENULL( (void **)&packed_channel_header );
	FREENULL( (void **)&packed_chaptern );
}

void channel_destroy( channel_t **channel )
{
	if( ! channel ) return;
	if( ! *channel ) return;

	if( (*channel)->chaptern )
	{
		chaptern_destroy( &( (*channel)->chaptern ) );
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
	chaptern_t *new_chaptern = NULL;
	
	new_channel = ( channel_t * ) malloc( sizeof( channel_t ) );

	if( ! new_channel ) return NULL;

	new_header = channel_header_create();

	if( ! new_header )
	{
		channel_destroy( &new_channel );
		return NULL;
	}

	new_channel->header = new_header;

	new_chaptern = chaptern_create();

	if( ! new_chaptern )
	{
		channel_destroy( &new_channel );
		return NULL;
	}

	new_channel->chaptern = new_chaptern;
		
	return new_channel;
}

void journal_pack( journal_t *journal, char **packed, size_t *size )
{
	char *packed_journal_header = NULL;
	size_t packed_journal_header_size = 0;
	char *packed_channel = NULL;
	size_t packed_channel_size = 0;
	char *packed_channel_buffer = NULL;	
	size_t packed_channel_buffer_size = 0;
	char *p = NULL;
	int i = 0;

	*packed = NULL;
	*size = 0;

	fprintf(stderr, ">journal_pack\n");
	if( ! journal ) return;

	journal_header_pack( journal->header, &packed_journal_header, &packed_journal_header_size );

	fprintf(stderr, "Packed Journal Header\n");
	hex_dump( packed_journal_header, packed_journal_header_size );

	for( i = 0 ; i < MAX_MIDI_CHANNELS ; i++ )
	{
		if( ! journal->channels[i] ) continue;

		channel_pack( journal->channels[i], &packed_channel, &packed_channel_size );

		packed_channel_buffer = ( char * )realloc( packed_channel_buffer, packed_channel_buffer_size + packed_channel_size );
		if( ! packed_channel_buffer ) goto journal_pack_cleanup;
		p = packed_channel_buffer + packed_channel_buffer_size;
		packed_channel_buffer_size += packed_channel_size;
		memcpy( packed_channel_buffer, packed_channel, packed_channel_size );

		FREENULL( (void **)&packed_channel );
	}

	// Join it all together

	*packed = ( char * )malloc( packed_journal_header_size + packed_channel_buffer_size );
	p = *packed;
	memcpy( p, packed_journal_header, packed_journal_header_size );
	*size += packed_journal_header_size;
	p += packed_journal_header_size;

	memcpy( p, packed_channel_buffer, packed_channel_buffer_size );
	*size += packed_channel_buffer_size;
	
journal_pack_cleanup:
	FREENULL( (void **)&packed_channel );
	FREENULL( (void **)&packed_channel_buffer );
	FREENULL( (void **)&packed_journal_header );
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

	// Set Journal Header A flag
	journal->header->bitfield |= 0x02;
	
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
		if( offset > journal->channels[channel - 1]->chaptern->header->high )
		{
			journal->channels[channel - 1]->chaptern->header->high = offset;
		}

		if( offset < journal->channels[channel - 1]->chaptern->header->low )
		{
			journal->channels[channel - 1]->chaptern->header->low = offset;
		}

		journal->header->seq = seq;
		journal->channels[channel - 1]->chaptern->offbits[offset] |=  ( 1 << shift );

		return;
	}

	if( journal->channels[ channel - 1 ]->chaptern->num_notes == MAX_CHAPTERN_NOTES ) return;

	new_note = midi_note_create();
	if(! new_note ) return;

	new_note->num = note;
	new_note->velocity = velocity;

	note_slot = journal->channels[ channel - 1]->chaptern->num_notes++;

	journal->channels[ channel - 1]->chaptern->notes[note_slot] = new_note;

	journal->channels[ channel - 1]->header->bitfield |= CHAPTER_N;
	journal->channels[ channel - 1]->chaptern->header->B = 1;

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

void chaptern_header_dump( chaptern_header_t *header )
{
	if( ! header ) return;

	fprintf(stderr, "Chapter N(header): B=%d len=%u low=%u high=%u\n", header->B, header->len, header->low, header->high);
}

void chaptern_dump( chaptern_t *chaptern )
{
	uint16_t i = 0;

	if( ! chaptern ) return;

	chaptern_header_dump( chaptern->header );

	for( i = 0 ; i < chaptern->num_notes ; i++ )
	{
		midi_note_dump( chaptern->notes[i] );
	}
	
	for( i = chaptern->header->low; i <= chaptern->header->high ; i++ )
	{
		fprintf(stderr, "Offbits[%d]=%02x\n", i, chaptern->offbits[i]);
	}
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
		chaptern_dump( channel->chaptern );
	}
}

void journal_header_dump( journal_header_t *header )
{
	if( ! header ) return;

	fprintf(stderr, "Journal (Header: bitfield=%02x totchan=%d seq=%04x)\n", header->bitfield, header->totchan, header->seq);
	fprintf(stderr, "Header Size = %u\n", sizeof( journal_header_t ) );
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
