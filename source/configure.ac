AC_INIT()
AM_INIT_AUTOMAKE(policy-milter, 0.0.11)

AC_PROG_CC
AC_PROG_INSTALL

AC_CHECK_HEADERS([pthread.h] ,, AC_MSG_ERROR([Cannot find pthread.h]))
AC_CHECK_LIB(pthread, pthread_self,,[ AC_MSG_ERROR([Cannot find libpthread])])

AC_CHECK_HEADERS([libmilter/mfapi.h],, AC_MSG_ERROR([Cannot find mfapi.h]))
AC_CHECK_LIB(milter, mi_stop,,[ AC_MSG_ERROR([Cannot find libmilter]) ] )

AC_OUTPUT(Makefile src/Makefile src/pm_version.h data/Makefile doc/Makefile policy-milter.spec)
