SUBDIRS = src man

CPPFLAGS = '-g -Wall -I ./include'
EXTRA_DIST = include LICENSE DEBIAN/control.in DEBIAN/control-lib.in DEBIAN/control-dev.in raveloxmidi.spec* raveloxmidi.pc.in

include_HEADERS = \
	include/applemidi_by.h \
	include/applemidi_feedback.h \
	include/applemidi_inv.h \
	include/applemidi_ok.h \
	include/applemidi_sync.h \
	include/chapter_c.h \
	include/chapter_n.h \
	include/chapter_p.h \
	include/daemon.h \
	include/data_context.h \
	include/data_queue.h \
	include/data_table.h \
	include/dbuffer.h \
	include/dns_service_discover.h \
	include/dns_service_publisher.h \
	include/dstring.h \
	include/kv_table.h \
	include/logging.h \
	include/midi_command.h \
	include/midi_control.h \
	include/midi_journal.h \
	include/midi_note.h \
	include/midi_payload.h \
	include/midi_program.h \
	include/midi_sender.h \
	include/midi_state.h \
	include/net_applemidi.h \
	include/net_connection.h \
	include/net_response.h \
	include/net_socket.h \
	include/raveloxmidi_alsa.h \
	include/raveloxmidi_config.h \
	include/remote_connection.h \
	include/ring_buffer.h \
	include/rtp_packet.h \
	include/utils.h

nodist_include_HEADERS = config.h include/build_info.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = raveloxmidi.pc

ACLOCAL_AMFLAGS = -I m4

CLEANFILES = *.tar.gz *spec build/*deb

deb:
	chmod 700 pkgscripts/build_deb
	@pkgscripts/build_deb

rpm:	dist
	rpmbuild -ta $(distdir).tar.gz
