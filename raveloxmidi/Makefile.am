SUBDIRS = src man pkgconfig include

EXTRA_DIST = include LICENSE DEBIAN/*.in raveloxmidi.spec* raveloxmidi.service.* pkgscripts/* pkgconfig/*

ACLOCAL_AMFLAGS = -I m4

CLEANFILES = *.tar.gz *spec build/* DEBIAN/*.control DEBIAN/*.files pkgscripts/build_deb

deb:
	chmod 700 pkgscripts/build_deb
	@pkgscripts/build_deb 

rpm:	dist
	@rpmbuild -ta $(distdir).tar.gz
	@rm -f $(distdir).tar.gz

what:
	@echo -n "Branch Name: "
	@git rev-parse --abbrev-ref HEAD
	@echo -n "Build number: "
	@git log --pretty=oneline | wc -l

