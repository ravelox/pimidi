bin_PROGRAMS = raveloxmidi

raveloxmidi_SOURCES = \
	raveloxmidi.c \
	dns_service_publisher.c \
	dns_service_discover.c \
	applemidi_inv.c \
	applemidi_by.c \
	applemidi_feedback.c \
	applemidi_sync.c \
	midi_journal.c \
	chapter_p.c \
	chapter_n.c \
	chapter_c.c \
	net_socket.c \
	net_response.c \
	midi_note.c \
	midi_control.c \
	midi_program.c \
	midi_payload.c \
	midi_command.c \
	net_applemidi.c \
	net_connection.c \
	rtp_packet.c \
	raveloxmidi_config.c \
	daemon.c \
	logging.c \
	utils.c \
	raveloxmidi_alsa.c

raveloxmidi_LDADD = @PTHREAD_LIBS@ @AVAHI_LIBS@ @ALSA_LIBS@
raveloxmidi_CFLAGS = @PTHREAD_CFLAGS@ @AVAHI_CFLAGS@ @ALSA_CFLAGS@

EXTRA_DIST = 

INCLUDES = -I ../include
