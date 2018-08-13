AC_INIT()
AC_CANONICAL_HOST
AC_CANONICAL_BUILD
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE(raveloxmidi, 0.5.0)
AC_CONFIG_HEADERS([config.h])

AC_GNU_SOURCE
AC_PROG_CC
AC_PROG_INSTALL

AC_SYS_LARGEFILE

ACX_PTHREAD([],[])

PKG_PROG_PKG_CONFIG

PKG_CHECK_MODULES(AVAHI, avahi-client )

AC_ARG_ENABLE(alsa,
        AC_HELP_STRING([--enable-alsa],[Enable alsa support]),
        enable_alsa="yes", enable_alsa="no")
if test "x$enable_alsa" == "xyes" -o "x$enable_all" == "xyes"
then
	PKG_CHECK_MODULES(ALSA,alsa,[have_alsa="yes"])
	if test "$have_alsa" == "yes"
	then
	   AC_DEFINE(HAVE_ALSA, 1, [ALSA has been detected])
	fi
fi

DEB_ARCH="`dpkg --print-architecture`"
AC_SUBST(DEB_ARCH)
AC_OUTPUT(Makefile src/Makefile man/Makefile man/raveloxmidi.1 DEBIAN/control pkgscripts/build_deb raveloxmidi.spec)
