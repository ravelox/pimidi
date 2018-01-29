AC_INIT()
AC_CANONICAL_HOST
AC_CANONICAL_BUILD
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE(raveloxmidi, 0.2.3)
AC_CONFIG_HEADERS([config.h])

AC_GNU_SOURCE
AC_PROG_CC
AC_PROG_INSTALL

AC_SYS_LARGEFILE

ACX_PTHREAD([],[])
PKG_CHECK_MODULES(AVAHI, avahi-client )

DEB_ARCH="`dpkg --print-architecture`"
AC_SUBST(DEB_ARCH)
AC_OUTPUT(Makefile src/Makefile man/Makefile man/raveloxmidi.1 DEBIAN/control pkgscripts/build_deb raveloxmidi.spec)
