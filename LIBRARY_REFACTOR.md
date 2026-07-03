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

- [ ] Define the public API boundary.
- [ ] Add a primary public SDK header, such as `raveloxmidi.h`.
- [ ] Identify internal headers that should no longer be installed.
- [ ] Keep only intentional public headers in the dev package.
- [ ] Introduce an opaque SDK context type, such as `raveloxmidi_context_t`.
- [ ] Add public lifecycle APIs for creating, starting, stopping and freeing a context.
- [ ] Move the `raveloxmidi` binary to use the public SDK API.
- [ ] Add a public version API, such as `raveloxmidi_version()`.
- [ ] Add symbol visibility controls for exported library APIs.
- [ ] Review libtool versioning and set an intentional ABI version policy.

## Phase 3 - MIDI Event Callbacks

- [ ] Define public callback types for MIDI events sent by `midi_sender`.
- [ ] Add callback registration and unregistration APIs.
- [ ] Include user data in callback registration.
- [ ] Ensure callbacks receive enough event metadata for useful integrations.
- [ ] Avoid running user callbacks on the sender hot path.
- [ ] Queue callback events to a dispatcher or worker thread.
- [ ] Define queue overflow behavior.
- [ ] Document callback threading and latency guarantees.
- [ ] Add tests or validation tooling for callback delivery.

## Phase 4 - Developer Experience

- [ ] Add a minimal C SDK example using `pkg-config`.
- [ ] Add an example that registers MIDI sent callbacks.
- [ ] Add an example that starts the service, sends a MIDI event and shuts down.
- [ ] Document native package builds.
- [ ] Document Docker package validation builds.
- [ ] Document runtime, library and dev package contents.
- [ ] Document what API is stable and what remains experimental.

## Phase 5 - Packaging Cleanup

- [ ] Ensure the runtime library package contains only runtime library files.
- [ ] Ensure the binary package contains only the CLI/service files.
- [ ] Ensure the dev package contains public headers, linker symlink and pkg-config files.
- [ ] Remove internal headers from package outputs unless they are intentionally public.
- [ ] Verify `.deb` package contents in Docker validation.
- [ ] Verify `.rpm` package contents in Docker validation.
