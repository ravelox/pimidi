#!/bin/sh
set -e

rm -f Makefile
rm -Rf autom4te.cache

mkdir -vp m4

if command -v libtoolize >/dev/null 2>&1
then
	libtoolize --copy --force
elif command -v glibtoolize >/dev/null 2>&1
then
	glibtoolize --copy --force
else
	echo "libtoolize is required to regenerate the build system" >&2
	exit 1
fi
aclocal
autoconf -f
autoheader
automake --foreign --add-missing --force-missing --copy
