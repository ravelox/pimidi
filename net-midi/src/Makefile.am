bin_PROGRAMS = net-midi avahi-test 

avahi_test_SOURCES = \
	avahi_test.c

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
	net_listener.c \
	rtp_packet.c \
	utils.c 

net_midi_LDADD	= 

net_midi_LDFLAGS = 

avahi_test_LDADD = ${AVAHI_LIBS}

EXTRA_DIST = 

INCLUDES = -I ../include
