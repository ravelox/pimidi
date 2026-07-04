#!/bin/bash
set -euo pipefail

cd /workspace/raveloxmidi

mkdir -p build
saved_debs="$(mktemp -d)"
cp -p build/*.deb "${saved_debs}/" 2>/dev/null || true
rm -rf /home/builder/rpmbuild/RPMS /home/builder/rpmbuild/SRPMS

./autogen.sh
./configure
make clean
mkdir -p build
cp -p "${saved_debs}/"*.deb build/ 2>/dev/null || true
rm -rf "${saved_debs}"
rm -f build/*.rpm
./config.status raveloxmidi.spec
make -j"$(nproc)"
make dist

tarball="$(ls -1t raveloxmidi-*.tar.gz | head -n 1)"
rpmbuild -ta "${tarball}"
find /home/builder/rpmbuild/RPMS /home/builder/rpmbuild/SRPMS -type f -name '*.rpm' -exec cp -p {} build/ \;
pkgscripts/validate_rpm_contents build/*.rpm

echo
echo "RPM artifacts:"
find build -maxdepth 1 -type f -name '*.rpm' -print | sort
