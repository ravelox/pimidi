bin_PROGRAMS = net_midi avahi_test  parse_test

avahi_test_SOURCES = \
	avahi_test.c

parse_test_SOURCES = \
	parse_test.c \
	cmd_end_handler.c \
	cmd_inv_handler.c \
	cmd_sync_handler.c \
	midi_journal.c \
	midi_note_packet.c \
	midi_payload.c \
	net_applemidi.c \
	net_connection.c \
	rtp_packet.c \
	net_listener.c \
	midi_command.c \
	utils.c

net_midi_SOURCES = \
	net_midi.c \
	cmd_end_handler.c \
	cmd_inv_handler.c \
	cmd_sync_handler.c \
	midi_journal.c \
	midi_note_packet.c \
	midi_payload.c \
	net_applemidi.c \
	net_connection.c \
	rtp_packet.c \
	net_listener.c \
	midi_command.c \
	utils.c 

net_midi_LDADD	= @PTHREAD_LIBS@

net_midi_LDFLAGS = @PTHREAD_CFLAGS@

avahi_test_LDADD = ${AVAHI_LIBS}

EXTRA_DIST = 

INCLUDES = -I ../include
