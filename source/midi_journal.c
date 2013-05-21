#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include "midi_journal.h"

static recovery_journal_header_t * new_recovery_journal_header( void )
{
	recovery_journal_header_t *journal_header = NULL;

	journal_header = ( recovery_journal_header_t * ) malloc( sizeof( recovery_journal_header_t ) );
	
	if( journal_header )
	{
		memset( journal_header , 0 , sizeof( recovery_journal_header_t ) );
	}

	return journal_header;
}


static channel_journal_header_t * new_channel_journal_header( void )
{
	channel_journal_header_t *channel_header = NULL;

	channel_header = ( channel_journal_header_t * ) malloc( sizeof( channel_journal_header_t ) );

	if( channel_header )
	{
		memset( channel_header, 0 , sizeof( channel_journal_header_t ) );
	}

	return channel_header;
}

static chapter_n_journal_header_t * new_chapter_n_journal_header( void )
{
	chapter_n_journal_header_t *chapter_n_header = NULL;

	chapter_n_header = ( chapter_n_journal_header_t *) malloc( sizeof( chapter_n_journal_header_t ) );

	if( chapter_n_header )
	{
		memset( chapter_n_header, 0 , sizeof( chapter_n_journal_header_t) );
	}

	return chapter_n_header;
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


chapter_n_journal_t * new_chapter_n_journal( void )
{
	chapter_n_journal_t *chaptern = NULL;

	chaptern = ( chapter_n_journal_t * ) malloc( sizeof( chapter_n_journal_t ) );

	if( chaptern )
	{
		chapter_n_journal_header_t *header = new_chapter_n_journal_header();

		if( ! header )
		{
			free( chaptern );
			return NULL;
		}

		chaptern->header = header;
		chaptern->notes = NULL;
	}

	return chaptern;
}

static void chapter_n_journal_destroy( chapter_n_journal_t *chapter_n )
{
	if( ! chapter_n )
	{
		return;
	}

	if( chapter_n->header )
	{
		free( chapter_n->header );
		chapter_n->header = NULL;
	}

	free( chapter_n );
	return;
}

recovery_journal_t * journal_init( void )
{

	recovery_journal_t *journal;
	recovery_journal_header_t *header;
	chapter_n_journal_t *chapter_n;

	journal = ( recovery_journal_t * ) malloc( sizeof ( recovery_journal_t ) );

	if( ! journal )
	{
		return NULL ;
	}

	header = new_recovery_journal_header();
	chapter_n = new_chapter_n_journal();
		
	if( ! header )
	{
		free( journal );
		return NULL;
	}

	if( ! chapter_n )
	{
		free( header );
		free( journal );
		return NULL;
	}
	
	journal->header = header;
	journal->chapter_n = chapter_n;

	return journal;
}

void journal_destroy( recovery_journal_t *journal )
{

	if( ! journal ) return;

	if( journal->chapter_n )
	{
		chapter_n_journal_destroy( journal->chapter_n );
		journal->chapter_n = NULL;
	}

	if( journal->header )
	{
		free( journal->header );
		journal->header = NULL;
	}

	free( journal );
	journal = NULL;
}
