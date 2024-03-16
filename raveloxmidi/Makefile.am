SUBDIRS = src man pkgconfig include

EXTRA_DIST = include LICENSE DEBIAN/*.in DEBIAN/*.files raveloxmidi.spec* raveloxmidi.service.* pkgscripts/* pkgconfig/*

ACLOCAL_AMFLAGS = -I m4

CLEANFILES = *.tar.gz *spec build/* DEBIAN/*.control pkgscripts/build_deb

deb:
	chmod 700 pkgscripts/build_deb
	@pkgscripts/build_deb 

rpm:	dist
	rpmbuild -ta $(distdir).tar.gz
