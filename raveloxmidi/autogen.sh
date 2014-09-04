#!/bin/sh

set -x

rm -f Makefile

aclocal
autoconf

autoheader

automake --foreign --add-missing --force-missing --copy
