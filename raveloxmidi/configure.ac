AC_INIT([raveloxmidi],[0.8.99])
AC_CANONICAL_HOST
AC_CANONICAL_BUILD
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE

AC_CONFIG_HEADERS([config.h])

AC_GNU_SOURCE
AC_PROG_CC
AC_PROG_INSTALL

AC_SYS_LARGEFILE

PKG_PROG_PKG_CONFIG

PKG_CHECK_MODULES(AVAHI, avahi-client )

ACX_PTHREAD([],[])

AC_CHECK_LIB(asound,snd_rawmidi_open,have_alsa="yes")
if test "$have_alsa" == "yes"
then
   ALSA_LIBS="`pkg-config --libs alsa`"
   ALSA_CFLAGS="`pkg-config --cflags alsa`"
   ALSA_DEPS=",libasound2"
   AC_SUBST(ALSA_LIBS)
   AC_SUBST(ALSA_CFLAGS)
   AC_SUBST(ALSA_DEPS)
   AC_DEFINE(HAVE_ALSA, 1, [ALSA has been detected])
fi

AC_CHECK_PROG([have_dpkg],[dpkg], "yes", "no")
if test "$have_dpkg" == "yes"
then
	DEB_ARCH="`dpkg --print-architecture`"
	AC_SUBST(DEB_ARCH)
	AC_OUTPUT( DEBIAN/control pkgscripts/build_deb )
fi

COPYRIGHT_YEAR="`date +'%Y'`"
AC_SUBST(COPYRIGHT_YEAR)

AC_OUTPUT(Makefile src/Makefile man/Makefile man/raveloxmidi.1 raveloxmidi.spec)
