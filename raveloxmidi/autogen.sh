#!/bin/sh

rm -f Makefile
rm -Rf autom4te.cache

aclocal
autoconf
autoheader
automake --foreign --add-missing --force-missing --copy
