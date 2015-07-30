bin_PROGRAMS = raveloxmidi

raveloxmidi_SOURCES = \
	raveloxmidi.c \
	dns_service_publisher.c \
	cmd_end_handler.c \
	cmd_inv_handler.c \
	cmd_sync_handler.c \
	cmd_feedback_handler.c \
	midi_journal.c \
	midi_note_packet.c \
	midi_payload.c \
	net_applemidi.c \
	net_connection.c \
	rtp_packet.c \
	net_listener.c \
	raveloxmidi_config.c \
	daemon.c \
	logging.c \
	utils.c 

raveloxmidi_LDADD = @PTHREAD_LIBS@ ${AVAHI_LIBS}

raveloxmidi_LDFLAGS = @PTHREAD_CFLAGS@

EXTRA_DIST = 

INCLUDES = -I ../include
