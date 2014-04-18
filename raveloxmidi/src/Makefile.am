bin_PROGRAMS = raveloxmidi dns_service_publisher  parse_test

dns_service_publisher_SOURCES = \
	dns_service_publisher.c

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
	utils.c

raveloxmidi_SOURCES = \
	raveloxmidi.c \
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
	utils.c 

raveloxmidi_LDADD	= @PTHREAD_LIBS@

raveloxmidi_LDFLAGS = @PTHREAD_CFLAGS@

dns_service_publisher_LDADD = ${AVAHI_LIBS}

EXTRA_DIST = 

INCLUDES = -I ../include
