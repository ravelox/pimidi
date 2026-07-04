#!/bin/sh

set -eu

IN_PIPE=${1:-/tmp/raveloxmidi-in}
OUT_PIPE=${2:-/tmp/raveloxmidi-out}
BIND_ADDRESS=${3:-0.0.0.0}

[ -p "$IN_PIPE" ] || mkfifo "$IN_PIPE"
[ -p "$OUT_PIPE" ] || mkfifo "$OUT_PIPE"

echo "Starting raveloxmidi-stream with:"
echo "  input:  $IN_PIPE"
echo "  output: $OUT_PIPE"
echo "  bind:   $BIND_ADDRESS"

raveloxmidi-stream --bind-address "$BIND_ADDRESS" --input "$IN_PIPE" --output "$OUT_PIPE"
