# Stream CLI Tools

`raveloxmidi-stream` bridges simple binary MIDI streams to and from the
`libraveloxmidi` SDK.

It can:

- read raw MIDI from stdin and send it over RTP-MIDI
- read raw MIDI from a named pipe and send it over RTP-MIDI
- write MIDI events received by the SDK to stdout
- write MIDI events received by the SDK to a named pipe

The utility uses the public SDK only. Input and output file descriptors
are handled on separate worker threads so slow pipes, stdin or stdout do
not run in the SDK MIDI event callback path.

## Usage

```sh
raveloxmidi-stream --bind-address 0.0.0.0 --input -
raveloxmidi-stream --bind-address 0.0.0.0 --output -
raveloxmidi-stream --config raveloxmidi.conf --input /tmp/raveloxmidi-in
raveloxmidi-stream --config raveloxmidi.conf --output /tmp/raveloxmidi-out
raveloxmidi-stream --config raveloxmidi.conf --input /tmp/raveloxmidi-in --output /tmp/raveloxmidi-out
```

Options:

- `-c`, `--config filename`: load a raveloxmidi config file.
- `-b`, `--bind-address address`: set `network.bind_address`.
- `-i`, `--input -|path`: read outgoing MIDI bytes from stdin or a
  file/named pipe.
- `-o`, `--output -|path`: write incoming MIDI bytes to stdout or a
  file/named pipe.
- `-d`, `--debug`: enable debug logging.
- `-v`, `--version`: print the version.
- `-h`, `--help`: print usage.

At least one of `--input` or `--output` is required.

## Binary MIDI Stream Format

The stream format is a sequence of ordinary MIDI messages:

```text
status [data...]
```

For channel voice messages:

- `0x8n` Note Off: status, note, velocity
- `0x9n` Note On: status, note, velocity
- `0xAn` Poly Pressure: status, note, pressure
- `0xBn` Control Change: status, controller, value
- `0xCn` Program Change: status, program
- `0xDn` Channel Pressure: status, pressure
- `0xEn` Pitch Bend: status, lsb, msb

For common system messages:

- `0xF1` MIDI Time Code Quarter Frame: status, value
- `0xF2` Song Position: status, lsb, msb
- `0xF3` Song Select: status, song
- single-byte real-time messages such as `0xF8`, `0xFA`, `0xFB`,
  `0xFC`, `0xFE` and `0xFF`

Running status is not supported. Each message must include its status
byte.

System Exclusive framing is not implemented in the stream utility yet.
`0xF0` and `0xF7` are currently treated as single-byte raw messages.

### SysEx Framing Implementation Plan

Implementing SysEx support requires a framed stream representation because
SysEx messages are variable-length and may contain arbitrary bytes until
the terminating `0xF7`.

Required steps:

- Define the stream contract for SysEx as `0xF0 payload... 0xF7`, where
  the full frame is delivered as one raw MIDI event. Document whether the
  output side always writes the terminating `0xF7` and how malformed input
  is handled.
- Replace the input thread's fixed two-byte `data_bytes` buffer with a
  dynamically sized message buffer.
- When input reads `0xF0`, continue reading bytes until `0xF7`, EOF,
  shutdown, or a configured maximum SysEx frame size.
- Reject or log malformed SysEx frames that hit EOF/shutdown before `0xF7`
  or exceed the maximum frame size.
- Extend `raveloxmidi_context_send_raw_midi()` or add a companion SDK call
  so callers can submit variable-length raw MIDI data beginning with
  `0xF0`. The current stream input path can only pass status plus a short
  fixed data payload.
- Update the SDK event callback/output path so `RAVELOXMIDI_EVENT_RAW_MIDI`
  events with SysEx payloads are queued and written without the current
  three-byte limit.
- Add tests for complete SysEx frames, malformed unterminated frames,
  oversized frames, and back-to-back channel/SysEx/channel messages.

## Examples

Send middle C Note On over RTP-MIDI from stdin:

```sh
printf '\x90\x3c\x64' | raveloxmidi-stream --bind-address 127.0.0.1 --input -
```

Print received MIDI events as hex:

```sh
raveloxmidi-stream --bind-address 0.0.0.0 --output - | hexdump -C
```

Use named pipes:

```sh
mkfifo /tmp/raveloxmidi-in /tmp/raveloxmidi-out
raveloxmidi-stream --bind-address 0.0.0.0 --input /tmp/raveloxmidi-in --output /tmp/raveloxmidi-out
```

In another terminal:

```sh
printf '\x90\x3c\x64' > /tmp/raveloxmidi-in
hexdump -C /tmp/raveloxmidi-out
```
