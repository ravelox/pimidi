#!/bin/sh

set -x

rm -f Makefile

libtoolize
aclocal
autoconf

autoheader

automake --foreign --add-missing --force-missing --copy
