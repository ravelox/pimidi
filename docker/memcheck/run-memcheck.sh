#!/bin/bash
set -euo pipefail

cd /workspace/raveloxmidi

./autogen.sh
./configure
make clean
make -j"$(nproc)"
make memcheck
