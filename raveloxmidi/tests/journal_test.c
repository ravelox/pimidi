#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "midi_journal.h"

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
	return 0;
}
