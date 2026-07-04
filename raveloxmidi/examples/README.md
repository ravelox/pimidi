# raveloxmidi SDK Examples

These examples are intended to be built against an installed
`libraveloxmidi` SDK package. Install `libraveloxmidi-dev` on Debian or
Ubuntu, or `libraveloxmidi-devel` on Fedora or other RPM-based systems.

Build all examples with:

```sh
make
```

The example Makefile uses:

```sh
pkg-config --cflags --libs raveloxmidi
```

## Examples

- `minimal_sdk.c` creates a context, reads the library version and sets a
  configuration item.
- `midi_event_callback.c` registers a MIDI event callback and runs until
  interrupted. It disables local inbound file output so applications can
  receive events through the callback only.
- `send_note_and_stop.c` starts the service, sends one Note On event and
  shuts down.

The service examples require a usable runtime environment, including a
running Avahi daemon and a valid `network.bind_address`.
