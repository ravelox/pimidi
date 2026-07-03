#!/bin/bash
set -euo pipefail

cd /workspace/raveloxmidi

./autogen.sh
./configure
make clean
make -j"$(nproc)"
make deb

echo
echo "Debian package metadata:"
for package in build/*.deb
do
	echo
	echo "${package}:"
	dpkg-deb --info "${package}"
done

echo
echo "Debian package artifacts:"
find build -maxdepth 1 -type f -name '*.deb' -print | sort
