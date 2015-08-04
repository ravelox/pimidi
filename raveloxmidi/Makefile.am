SUBDIRS = src man

CPPFLAGS = '-g -Wall -I ./include'
EXTRA_DIST = include LICENSE DEBIAN/control

ACLOCAL_AMFLAGS = -I m4

deb:
	chmod 700 pkgscripts/build_deb
	@pkgscripts/build_deb
