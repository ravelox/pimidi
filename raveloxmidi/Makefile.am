SUBDIRS = src tests man

CPPFLAGS = '-g -Wall -I ./include'
EXTRA_DIST = include docs examples LICENSE DEBIAN/control.in DEBIAN/control-lib.in DEBIAN/control-dev.in pkgscripts/branch_name pkgscripts/build_number pkgscripts/validate_deb_contents pkgscripts/validate_rpm_contents raveloxmidi.spec* raveloxmidi.pc.in

include_HEADERS = \
	include/raveloxmidi.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = raveloxmidi.pc

ACLOCAL_AMFLAGS = -I m4

CLEANFILES = *.tar.gz *spec configure~

distclean-local:
	rm -rf build

deb:
	chmod 700 pkgscripts/build_deb
	@pkgscripts/build_deb

rpm:	dist
	rpmbuild -ta $(distdir).tar.gz

docker-deb:
	docker compose -f ../docker/debbuild/compose.yaml up --build --abort-on-container-exit

docker-rpm:
	docker compose -f ../docker/rpmbuild/compose.yaml up --build --abort-on-container-exit

docker-packages: docker-deb docker-rpm

memcheck: all
	$(MAKE) -C tests memcheck

docker-memcheck:
	docker compose -f ../docker/memcheck/compose.yaml up --build --abort-on-container-exit
