#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "midi_journal.h"
#include "midi_sender.h"

static int contains(const unsigned char *data, size_t size, const unsigned char *wanted, size_t wanted_size)
{
	size_t i;
	for (i = 0; i + wanted_size <= size; i++)
		if (memcmp(data + i, wanted, wanted_size) == 0) return 1;
	return 0;
}

int main(void)
{
	journal_t *journal = NULL;
	midi_command_t command;
	midi_note_t note;
	char *packed = NULL;
	size_t packed_size = 0, recovered_size = 0;
	unsigned char *recovered = NULL;
	const unsigned char pitch[] = { 0xe2, 1, 64 };
	const unsigned char pressure[] = { 0xd2, 55 };
	const unsigned char poly[] = { 0xa2, 60, 45 };
	const unsigned char off[] = { 0x82, 64, 0 };
	const unsigned char system_only[] = { 0x40, 0, 1, 0xc0, 0x04, 0xaa, 0x55 };
	unsigned char *with_system = NULL;
	unsigned char *enhanced_then_known = NULL;
	size_t with_system_size;

	assert(journal_init(&journal) == 0);
	memset(&command, 0, sizeof(command));
	command.status = 0xe2; command.data = (unsigned char *)pitch + 1; command.data_len = 2;
	midi_journal_add_command(journal, 10, &command);
	command.status = 0xd2; command.data = (unsigned char *)pressure + 1; command.data_len = 1;
	midi_journal_add_command(journal, 11, &command);
	command.status = 0xa2; command.data = (unsigned char *)poly + 1; command.data_len = 2;
	midi_journal_add_command(journal, 12, &command);
	memset(&note, 0, sizeof(note)); note.channel = 2; note.note = 64; note.command = MIDI_COMMAND_NOTE_OFF;
	midi_journal_add_note(journal, 13, &note);
	journal_pack(journal, &packed, &packed_size);
	assert(packed && packed_size > 3);
	assert(midi_journal_recover((unsigned char *)packed, packed_size, &recovered, &recovered_size));
	assert(contains(recovered, recovered_size, pitch, sizeof(pitch)));
	assert(contains(recovered, recovered_size, pressure, sizeof(pressure)));
	assert(contains(recovered, recovered_size, poly, sizeof(poly)));
	assert(contains(recovered, recovered_size, off, sizeof(off)));
	free(recovered); recovered = NULL; recovered_size = 0;

	/* Unknown system chapters are skipped using the system LENGTH field. */
	assert(midi_journal_recover(system_only, sizeof(system_only), &recovered, &recovered_size));
	assert(recovered == NULL && recovered_size == 0);
	with_system_size = packed_size + 4;
	with_system = malloc(with_system_size);
	assert(with_system);
	memcpy(with_system, packed, 3);
	with_system[0] |= 0x40;
	memcpy(with_system + 3, system_only + 3, 4);
	memcpy(with_system + 7, packed + 3, packed_size - 3);
	assert(midi_journal_recover(with_system, with_system_size, &recovered, &recovered_size));
	assert(contains(recovered, recovered_size, pitch, sizeof(pitch)));
	free(recovered); recovered = NULL; recovered_size = 0;

	/* Enhanced Chapter C has different leaf framing: skip its whole channel. */
	with_system[7] |= 0x04;
	assert(midi_journal_recover(with_system, with_system_size, &recovered, &recovered_size));
	assert(recovered == NULL && recovered_size == 0);
	enhanced_then_known = malloc(with_system_size + packed_size - 3);
	assert(enhanced_then_known);
	memcpy(enhanced_then_known, with_system, with_system_size);
	enhanced_then_known[0] = (enhanced_then_known[0] & 0xf0) | 1; /* two channels */
	memcpy(enhanced_then_known + with_system_size, packed + 3, packed_size - 3);
	assert(midi_journal_recover(enhanced_then_known, with_system_size + packed_size - 3,
		&recovered, &recovered_size));
	assert(contains(recovered, recovered_size, pitch, sizeof(pitch)));
	free(recovered); recovered = NULL; recovered_size = 0;

	/* Invalid enclosing lengths are rejected without returning partial MIDI. */
	with_system[4] = 0xff;
	assert(!midi_journal_recover(with_system, with_system_size, &recovered, &recovered_size));
	assert(recovered == NULL && recovered_size == 0);

	free(enhanced_then_known); free(with_system); free(packed); journal_destroy(&journal);

	/* Checkpoint remains at the oldest unacknowledged command and Chapter P restores banks. */
	{
		midi_control_t control;
		midi_program_t program;
		const unsigned char bank_msb[] = {0xb0,0,5}, bank_lsb[] = {0xb0,32,6}, program_change[] = {0xc0,7};
		assert(journal_init(&journal)==0);
		memset(&control,0,sizeof(control)); control.channel=0; control.controller_number=0; control.controller_value=5;
		midi_journal_add_control(journal,100,&control);
		control.controller_number=32; control.controller_value=6; midi_journal_add_control(journal,101,&control);
		memset(&program,0,sizeof(program)); program.channel=0; program.program=7; midi_journal_add_program(journal,105,&program);
		journal_pack(journal,&packed,&packed_size);
		assert(midi_journal_checkpoint_covers((unsigned char *)packed,packed_size,99));
		assert(!midi_journal_checkpoint_covers((unsigned char *)packed,packed_size,98));
		assert(midi_journal_should_recover(99,102,(unsigned char *)packed,packed_size));
		assert(!midi_journal_should_recover(99,100,(unsigned char *)packed,packed_size));
		assert(!midi_journal_should_recover(102,99,(unsigned char *)packed,packed_size));
		packed[1]=0xff; packed[2]=0xff; assert(midi_journal_checkpoint_covers((unsigned char *)packed,packed_size,0xfffe));
		packed[1]=0; packed[2]=0; assert(midi_journal_checkpoint_covers((unsigned char *)packed,packed_size,0xffff));
		assert(midi_journal_should_recover(0xffff,2,(unsigned char *)packed,packed_size));
		assert(midi_journal_recover((unsigned char *)packed,packed_size,&recovered,&recovered_size));
		assert(contains(recovered,recovered_size,bank_msb,sizeof(bank_msb)));
		assert(contains(recovered,recovered_size,bank_lsb,sizeof(bank_lsb)));
		assert(contains(recovered,recovered_size,program_change,sizeof(program_change)));
		free(recovered); free(packed); recovered=NULL; packed=NULL; journal_destroy(&journal);
	}

	/* Segmented SysEx state is joined before Chapter X is emitted. */
	{
		const unsigned char first[]={0x7d,1,0xf0}, middle[]={2,3,0xf0}, last[]={4,0xf7};
		const unsigned char expected[]={0xf0,0x7d,1,2,3,4,0xf7};
		assert(journal_init(&journal)==0); memset(&command,0,sizeof(command));
		command.status=0xf0; command.data=(unsigned char *)first; command.data_len=sizeof(first); midi_journal_add_command(journal,300,&command);
		assert(!journal_has_data(journal));
		command.status=0xf7; command.data=(unsigned char *)middle; command.data_len=sizeof(middle); midi_journal_add_command(journal,301,&command);
		command.data=(unsigned char *)last; command.data_len=sizeof(last); midi_journal_add_command(journal,302,&command);
		journal_pack(journal,&packed,&packed_size); assert(packed);
		assert(midi_journal_recover((unsigned char *)packed,packed_size,&recovered,&recovered_size));
		assert(recovered_size==sizeof(expected)&&memcmp(recovered,expected,sizeof(expected))==0);
		free(recovered); free(packed); recovered=NULL; packed=NULL; journal_destroy(&journal);
	}

	/* Chapter X cannot exceed its enclosing 10-bit system-journal length. */
	{
		unsigned char *large=calloc(1022,1); assert(large); large[1021]=0xf7;
		assert(journal_init(&journal)==0); memset(&command,0,sizeof(command));
		command.status=0xf0; command.data=large; command.data_len=1022; midi_journal_add_command(journal,400,&command);
		journal_pack(journal,&packed,&packed_size); assert(packed==NULL&&packed_size==0);
		free(large); journal_destroy(&journal);
	}

	/* A finished, bounded SysEx command round-trips through system Chapter X. */
	{
		const unsigned char sysex_data[]={0x7d,1,2,0xf7};
		const unsigned char sysex_wire[]={0xf0,0x7d,1,2,0xf7};
		assert(journal_init(&journal)==0);
		memset(&command,0,sizeof(command)); command.status=0xf0; command.data=(unsigned char *)sysex_data; command.data_len=sizeof(sysex_data);
		midi_journal_add_command(journal,200,&command);
		journal_pack(journal,&packed,&packed_size);
		assert(packed && (packed[0]&0x40));
		assert(midi_journal_recover((unsigned char *)packed,packed_size,&recovered,&recovered_size));
		assert(contains(recovered,recovered_size,sysex_wire,sizeof(sysex_wire)));
		free(recovered); free(packed); recovered=NULL; packed=NULL; journal_destroy(&journal);
	}

	/* Batched command lists retain an explicit status for every command. */
	{
		midi_command_t *commands[2]; unsigned char *batch=NULL; size_t batch_size=0;
		const unsigned char first[]={60,100}, second[]={61,0}, expected[]={0x90,60,100,0x80,61,0};
		commands[0]=midi_command_create(); commands[1]=midi_command_create(); assert(commands[0]&&commands[1]);
		midi_command_set(commands[0],0,0x90,first,sizeof(first)); midi_command_set(commands[1],0,0x80,second,sizeof(second));
		assert(midi_sender_pack_commands(commands,2,&batch,&batch_size));
		assert(batch_size==sizeof(expected)&&memcmp(batch,expected,sizeof(expected))==0);
		free(batch); midi_command_destroy((void **)&commands[0]); midi_command_destroy((void **)&commands[1]);
	}
	return 0;
}
