# Library Refactor Tracking

This document tracks the remaining work to turn raveloxmidi into a
standalone library with a stable SDK/API while keeping the raveloxmidi
binary as a consumer of that library.

## Phase 1 - Build And Packaging Split

- [x] Build core functionality as `libraveloxmidi`.
- [x] Link the `raveloxmidi` binary against `libraveloxmidi`.
- [x] Install headers and pkg-config metadata.
- [x] Split native package outputs into binary, runtime library and dev packages.
- [x] Add Docker validation builds for Ubuntu `.deb` packages.
- [x] Add Docker validation builds for Fedora `.rpm` packages.
- [x] Copy Docker-built package artifacts to `raveloxmidi/build/`.
- [x] Keep package artifacts during `make clean`.
- [x] Remove package artifacts during `make distclean`.

## Phase 2 - Public SDK/API

- [x] P2-001 Define the public API boundary.
- [x] P2-002 Add a primary public SDK header, such as `raveloxmidi.h`.
- [x] P2-003 Identify internal headers that should no longer be installed.
- [x] P2-004 Keep only intentional public headers in the dev package.
- [x] P2-005 Introduce an opaque SDK context type, such as `raveloxmidi_context_t`.
- [x] P2-006 Add public lifecycle APIs for creating, starting, stopping and freeing a context.
- [x] P2-007 Move the `raveloxmidi` binary to use the public SDK API.
- [x] P2-008 Add a public version API, such as `raveloxmidi_version()`.
- [x] P2-009 Add symbol visibility controls for exported library APIs.
- [x] P2-010 Review libtool versioning and set an intentional ABI version policy.

## Phase 3 - MIDI Event Callbacks And Local I/O Separation

- [x] P3-001 Define public callback types for MIDI events sent by `midi_sender`.
- [x] P3-002 Add callback registration and unregistration APIs.
- [x] P3-003 Include user data in callback registration.
- [x] P3-004 Ensure callbacks receive enough event metadata for useful integrations.
- [x] P3-005 Avoid running user callbacks on the sender hot path.
- [x] P3-006 Queue callback events to a dispatcher or worker thread.
- [x] P3-007 Define queue overflow behavior.
- [x] P3-008 Document callback threading and latency guarantees.
- [x] P3-009 Add tests or validation tooling for callback delivery.
- [x] P3-010 Support callback-only operation with no ALSA output or inbound MIDI file sink.
- [x] P3-011 Separate RTP-MIDI networking from local MIDI input/output sinks.
- [x] P3-012 Add SDK APIs for inbound RTP-MIDI event callbacks.
- [x] P3-013 Add SDK APIs for outbound/local MIDI event injection.
- [x] P3-014 Define local MIDI source and sink modes: ALSA, file, stdin/stdout, named pipe and callback.

## Phase 4 - Developer Experience

- [x] Add a minimal C SDK example using `pkg-config`.
- [x] Add an example that registers MIDI sent callbacks.
- [x] Add an example that starts the service, sends a MIDI event and shuts down.
- [x] Document native package builds.
- [x] Document Docker package validation builds.
- [x] Document runtime, library and dev package contents.
- [x] Document what API is stable and what remains experimental.

## Phase 5 - Stream CLI Tools

- [ ] P5-001 Add a command line utility that reads raw MIDI from stdin and sends it over RTP-MIDI.
- [ ] P5-002 Add support for writing received MIDI events to stdout.
- [ ] P5-003 Add named pipe input for outgoing MIDI messages.
- [ ] P5-004 Add named pipe output for incoming MIDI messages.
- [ ] P5-005 Document the binary MIDI stream format.
- [ ] P5-006 Add examples for stdin/stdout and named pipe workflows.
- [ ] P5-007 Ensure stream utility behavior is implemented through the public SDK.
- [ ] P5-008 Ensure named pipe and stdin/stdout handling does not block sender or network threads.

## Phase 6 - Packaging Cleanup

- [ ] Ensure the runtime library package contains only runtime library files.
- [ ] Ensure the binary package contains only the CLI/service files.
- [ ] Ensure the dev package contains public headers, linker symlink and pkg-config files.
- [ ] Remove internal headers from package outputs unless they are intentionally public.
- [ ] Verify `.deb` package contents in Docker validation.
- [ ] Verify `.rpm` package contents in Docker validation.
