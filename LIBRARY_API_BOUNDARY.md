# Library API Boundary

This document defines the intended public API boundary for
`libraveloxmidi`. It is the completion record for `P2-001` in
`LIBRARY_REFACTOR.md`. The primary SDK header described here was added
as `raveloxmidi/include/raveloxmidi.h` for `P2-002`.

The goal is to provide a stable SDK for applications while allowing the
current protocol, socket, queue and daemon internals to keep evolving.

## Public API Policy

The public API is the set of declarations included from the primary SDK
header:

```c
#include <raveloxmidi.h>
```

Future headers installed for SDK consumers must be included by
`raveloxmidi.h` or explicitly documented as public. Headers not included
by the public SDK header are private implementation detail, even if they
are temporarily installed by the current build.

The public API must:

- Use the `raveloxmidi_` prefix for exported functions, types and macros.
- Expose opaque handles for long-lived library objects.
- Avoid exposing internal struct layouts.
- Avoid exposing pthread, socket, Avahi, ALSA or libtool implementation
  details in public types.
- Return explicit status codes for recoverable errors.
- Accept caller-owned `user_data` for callbacks.
- Document ownership of every pointer crossing the API boundary.

## Initial Public Surface

The Phase 2 SDK should expose these categories through
`raveloxmidi.h`.

### Version

- Library version query.
- Runtime ABI/API compatibility query if needed.

### Context Lifecycle

- Create and destroy a `raveloxmidi_context_t`.
- Start and stop the RTP-MIDI service.
- Apply configuration before start.

`raveloxmidi_context_t` must be opaque to callers.

### Configuration

Configuration should be passed through SDK functions or a small public
configuration object. The current global `config_*` functions are
implementation detail and should not become the SDK boundary.

### MIDI Events

The SDK should provide public MIDI event/value types for:

- Note on/off.
- Control change.
- Program change.
- Raw MIDI command bytes when needed.

These public event types should not reuse the current internal structs
unless their layout is intentionally frozen.

### Callbacks

The SDK should expose callback registration for MIDI events sent by
`midi_sender`. User callbacks must not run directly on the sender hot
path. Callback dispatch should use a queue or worker thread so slow user
code cannot add latency to delivery for other destinations.

## Private Implementation Surface

The following current header groups are private unless a later task
explicitly promotes a type or function into the SDK.

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

These headers describe packet formats, recovery journal state and
responder internals. They should remain changeable without breaking the
SDK.

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

These headers manage process state, sockets, discovery, connections,
ALSA and global configuration. Public SDK functions should wrap this
behavior behind `raveloxmidi_context_t`.

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
- generated `build_info.h`
- generated `config.h`

These headers are support utilities and generated build state. They
must not be required by SDK consumers.

## Candidate Public Types

The SDK should introduce new public types instead of freezing existing
internal structs too early:

```c
typedef struct raveloxmidi_context raveloxmidi_context_t;

typedef enum raveloxmidi_status {
	RAVELOXMIDI_OK = 0,
	RAVELOXMIDI_ERROR,
	RAVELOXMIDI_ERROR_INVALID_ARGUMENT,
	RAVELOXMIDI_ERROR_NO_MEMORY,
	RAVELOXMIDI_ERROR_NOT_RUNNING
} raveloxmidi_status_t;

typedef enum raveloxmidi_event_type {
	RAVELOXMIDI_EVENT_NOTE_OFF,
	RAVELOXMIDI_EVENT_NOTE_ON,
	RAVELOXMIDI_EVENT_CONTROL_CHANGE,
	RAVELOXMIDI_EVENT_PROGRAM_CHANGE,
	RAVELOXMIDI_EVENT_RAW_MIDI
} raveloxmidi_event_type_t;
```

Exact names and fields can be refined in later P2 and P3 tasks.

## Packaging Implications

Until P2-003 and P2-004 are complete, the build may still install
internal headers. That is a temporary compatibility state.

The intended final dev package contents are:

- `raveloxmidi.h`
- Any intentionally public companion headers.
- `libraveloxmidi.so` linker symlink.
- `raveloxmidi.pc`.
- SDK examples or documentation, if packaged.

The dev package should not require consumers to include internal headers
to perform supported SDK operations.
