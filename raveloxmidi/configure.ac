AC_INIT()
AM_INIT_AUTOMAKE(raveloxmidi, 0.0.1)
AC_CONFIG_HEADERS([config.h])

AM_MAINTAINER_MODE
AC_CANONICAL_HOST

AC_GNU_SOURCE
AC_PROG_CC
AC_PROG_INSTALL

AC_SYS_LARGEFILE

ACX_PTHREAD([],[])
PKG_CHECK_MODULES(AVAHI, [avahi-client] )

AC_OUTPUT(Makefile src/Makefile man/Makefile man/raveloxmidi.1 debian/control)
