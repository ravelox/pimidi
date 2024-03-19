lib_LTLIBRARIES = libraveloxmidi.la
bin_PROGRAMS = raveloxmidi

DEBUG_FLAGS = -Wall -Werror -g -O0
libraveloxmidi_la_CFLAGS=-fPIC @PTHREAD_CFLAGS@ @AVAHI_CFLAGS@ @ALSA_CFLAGS@ ${DEBUG_FLAGS}
libraveloxmidi_la_CPPFLAGS=-fPIC -I ../include  ${DEBUG_FLAGS}
libraveloxmidi_la_CXXFLAGS=-fPIC -I ../include ${DEBUG_FLAGS}
libraveloxmidi_la_LDFLAGS= -shared -fPIC
libraveloxmidi_la_SOURCES = \
	dns_service_publisher.c \
	dns_service_discover.c \
	applemidi_responder.c \
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
	rvxmidi.c 

libraveloxmidi_la_LIBADD = @PTHREAD_LIBS@ @AVAHI_LIBS@ @ALSA_LIBS@

raveloxmidi_SOURCES = \
	daemon.c \
	raveloxmidi.c 

raveloxmidi_LDADD = libraveloxmidi.la
raveloxmidi_CPPFLAGS = -I ../include ${DEBUG_FLAGS}

EXTRA_DIST = 
