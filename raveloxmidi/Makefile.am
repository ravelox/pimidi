SUBDIRS = src man

CPPFLAGS = '-g -Wall -I ./include'
EXTRA_DIST = include LICENSE DEBIAN/control raveloxmidi.spec*

ACLOCAL_AMFLAGS = -I m4

deb:
	chmod 700 pkgscripts/build_deb
	@pkgscripts/build_deb
