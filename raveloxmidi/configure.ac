m4_define([VERSION_NUMBER],[0.33.2])
m4_define([BUILD_NUMBER],m4_esyscmd_s(pkgscripts/build_number))
m4_define([FULL_VERSION], [VERSION_NUMBER.BUILD_NUMBER])
AC_INIT([raveloxmidi],[FULL_VERSION])
AC_CONFIG_MACRO_DIRS([m4])

AC_CANONICAL_HOST
AC_CANONICAL_BUILD
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE

AC_CONFIG_HEADERS([config.h])

AC_USE_SYSTEM_EXTENSIONS
AC_PROG_CC
AC_PROG_INSTALL
AM_PROG_AR
LT_INIT

AC_SYS_LARGEFILE

AC_CACHE_CHECK([whether $CC supports -fvisibility=hidden],
	[raveloxmidi_cv_cc_visibility_hidden],
	[
		save_CFLAGS="$CFLAGS"
		CFLAGS="$CFLAGS -fvisibility=hidden"
		AC_COMPILE_IFELSE(
			[AC_LANG_PROGRAM([], [])],
			[raveloxmidi_cv_cc_visibility_hidden=yes],
			[raveloxmidi_cv_cc_visibility_hidden=no])
		CFLAGS="$save_CFLAGS"
	])

AS_IF([test "x$raveloxmidi_cv_cc_visibility_hidden" = "xyes"],
	[RAVELOXMIDI_HIDDEN_VISIBILITY_CFLAGS="-fvisibility=hidden"],
	[RAVELOXMIDI_HIDDEN_VISIBILITY_CFLAGS=""])
AC_SUBST(RAVELOXMIDI_HIDDEN_VISIBILITY_CFLAGS)

PKG_PROG_PKG_CONFIG

PKG_CHECK_MODULES(AVAHI, avahi-client)

ACX_PTHREAD([],[])

AC_CHECK_LIB(asound,snd_rawmidi_open,have_alsa="yes",AC_MSG_WARN([ALSA library not found. Check raveloxmidi/required for the list of packages to install.]))
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
	AC_CONFIG_FILES([DEBIAN/control DEBIAN/control-lib DEBIAN/control-dev pkgscripts/build_deb ])
fi

COPYRIGHT_YEAR="`date +'%Y'`"
AC_SUBST(COPYRIGHT_YEAR)

GIT_BRANCH_NAME="`pkgscripts/branch_name`"
AC_SUBST(GIT_BRANCH_NAME)

AC_CONFIG_FILES([Makefile src/Makefile tests/Makefile man/Makefile man/raveloxmidi.1 man/raveloxmidi-stream.1 raveloxmidi.service raveloxmidi.spec raveloxmidi.pc include/build_info.h])
AC_OUTPUT
