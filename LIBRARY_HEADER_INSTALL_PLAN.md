# Library Header Install Plan

This document identifies the header install changes required for the
public SDK boundary. It is the completion record for `P2-003` in
`LIBRARY_REFACTOR.md`.

## Intended Public Headers

The dev package should install this public SDK header:

- `raveloxmidi.h`

No other header is currently approved as public SDK surface. Future
public companion headers must be added intentionally and included by, or
documented alongside, `raveloxmidi.h`.

## Headers To Stop Installing

The following currently installed headers are implementation detail and
should be removed from the install list in `P2-004`.

### Protocol Internals

- `applemidi_by.h`
- `applemidi_feedback.h`
- `applemidi_inv.h`
- `applemidi_ok.h`
- `applemidi_sync.h`
- `chapter_c.h`
- `chapter_n.h`
- `chapter_p.h`
- `midi_journal.h`
- `midi_payload.h`
- `net_applemidi.h`
- `net_response.h`
- `rtp_packet.h`

### Runtime Internals

- `daemon.h`
- `dns_service_discover.h`
- `dns_service_publisher.h`
- `midi_sender.h`
- `midi_state.h`
- `net_connection.h`
- `net_socket.h`
- `remote_connection.h`
- `raveloxmidi_alsa.h`
- `raveloxmidi_config.h`

### MIDI Implementation Internals

- `midi_command.h`
- `midi_control.h`
- `midi_note.h`
- `midi_program.h`

These currently expose internal struct layouts. Public MIDI value types
should be introduced through `raveloxmidi.h` instead of freezing these
layouts.

### Utility Internals

- `data_context.h`
- `data_queue.h`
- `data_table.h`
- `dbuffer.h`
- `dstring.h`
- `kv_table.h`
- `logging.h`
- `ring_buffer.h`
- `utils.h`

### Generated Build Internals

- `build_info.h`
- `config.h`

These generated headers should not be installed for SDK consumers.

## P2-004 Implementation Target

`P2-004` updates the build so that:

- `include_HEADERS` installs only `include/raveloxmidi.h`.
- `nodist_include_HEADERS` no longer installs generated internal headers.
- Internal headers remain available to the project build through local
  include paths.
- `.deb` and `.rpm` dev packages contain only intentional public headers.
