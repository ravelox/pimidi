SUBDIRS = src man pkgconfig include

EXTRA_DIST = include LICENSE DEBIAN/control raveloxmidi.spec* raveloxmidi.service.* pkgscripts/* pkgconfig/*

ACLOCAL_AMFLAGS = -I m4

CLEANFILES = *.tar.gz *spec build/*deb

deb:
	chmod 700 pkgscripts/build_deb
	@pkgscripts/build_deb

rpm:	dist
	rpmbuild -ta $(distdir).tar.gz
