lib_LTLIBRARIES = libraveloxmidi.la

LIBRAVELOXMIDI_LT_CURRENT = 2
LIBRAVELOXMIDI_LT_REVISION = 0
LIBRAVELOXMIDI_LT_AGE = 2

libraveloxmidi_la_SOURCES = \
	dns_service_publisher.c \
	dns_service_discover.c \
	applemidi_inv.c \
	applemidi_ok.c \
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
	ring_buffer.c \
	midi_state.c \
	net_applemidi.c \
	net_connection.c \
	remote_connection.c \
	rtp_packet.c \
	raveloxmidi_config.c \
	daemon.c \
	logging.c \
	utils.c \
	raveloxmidi_alsa.c \
	kv_table.c \
	data_table.c \
	dbuffer.c \
	dstring.c \
	data_queue.c \
	data_context.c \
	midi_sender.c \
	raveloxmidi_events.c \
	raveloxmidi_api.c

libraveloxmidi_la_LIBADD = @PTHREAD_LIBS@ @AVAHI_LIBS@ @ALSA_LIBS@
libraveloxmidi_la_CFLAGS = -DRAVELOXMIDI_BUILDING_LIBRARY @PTHREAD_CFLAGS@ @AVAHI_CFLAGS@ @ALSA_CFLAGS@
libraveloxmidi_la_LDFLAGS = -version-info $(LIBRAVELOXMIDI_LT_CURRENT):$(LIBRAVELOXMIDI_LT_REVISION):$(LIBRAVELOXMIDI_LT_AGE)

bin_PROGRAMS = raveloxmidi raveloxmidi-stream

raveloxmidi_SOURCES = raveloxmidi.c
raveloxmidi_LDADD = libraveloxmidi.la
raveloxmidi_CFLAGS = @PTHREAD_CFLAGS@ @AVAHI_CFLAGS@ @ALSA_CFLAGS@

raveloxmidi_stream_SOURCES = raveloxmidi_stream.c
raveloxmidi_stream_LDADD = libraveloxmidi.la @PTHREAD_LIBS@
raveloxmidi_stream_CFLAGS = @PTHREAD_CFLAGS@

EXTRA_DIST = 

AM_CPPFLAGS = -I ../include
