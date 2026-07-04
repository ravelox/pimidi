# Public SDK API

This document describes the public SDK calls currently available through
`raveloxmidi.h`.

Applications should include only the public SDK header:

```c
#include <raveloxmidi.h>
```

Internal project headers are not public API and should not be included
by SDK consumers.

## Status Values

Public SDK calls that can fail return `raveloxmidi_status_t`.

- `RAVELOXMIDI_OK`: The operation completed successfully.
- `RAVELOXMIDI_ERROR`: A general runtime failure occurred.
- `RAVELOXMIDI_ERROR_INVALID_ARGUMENT`: A required argument was missing
  or invalid.
- `RAVELOXMIDI_ERROR_NO_MEMORY`: Memory allocation failed.
- `RAVELOXMIDI_ERROR_INVALID_STATE`: The call is not valid for the
  current context state.
- `RAVELOXMIDI_ERROR_NOT_RUNNING`: The context is not running.
- `RAVELOXMIDI_ERROR_NOT_FOUND`: The requested item was not found.

## Context Lifetime

`raveloxmidi_context_t` is an opaque handle owned by the SDK caller.
Callers create it with `raveloxmidi_context_create()` and release it
with `raveloxmidi_context_free()`.

The current implementation supports one active context per process.
Creating a second context while another context exists returns
`RAVELOXMIDI_ERROR_INVALID_STATE`.

The usual call order is:

```c
raveloxmidi_context_t *context = NULL;

raveloxmidi_context_create( &context );
raveloxmidi_context_set_config_file( context, "/etc/raveloxmidi.conf" );
raveloxmidi_context_set_config( context, "network.bind_address", "127.0.0.1" );
raveloxmidi_context_start( context );
raveloxmidi_context_wait( context );
raveloxmidi_context_free( &context );
```

## `raveloxmidi_version`

```c
const char *raveloxmidi_version( void );
```

Returns the library version string compiled into `libraveloxmidi`.

The returned pointer is owned by the library. Callers must not modify or
free it.

This function does not require a context.

## `raveloxmidi_context_create`

```c
raveloxmidi_status_t raveloxmidi_context_create( raveloxmidi_context_t **context );
```

Creates a new SDK context and initializes the library subsystems needed
before configuration and startup.

Arguments:

- `context`: Output pointer that receives the new context. The pointer
  must not be `NULL`, and `*context` must be `NULL`.

Return values:

- `RAVELOXMIDI_OK`: Context created.
- `RAVELOXMIDI_ERROR_INVALID_ARGUMENT`: `context` is `NULL` or already
  points to a context.
- `RAVELOXMIDI_ERROR_INVALID_STATE`: Another context is already active.
- `RAVELOXMIDI_ERROR_NO_MEMORY`: Allocation failed.

Ownership:

- On success, the caller owns the returned context and must eventually
  call `raveloxmidi_context_free()`.
- On failure, no context ownership is transferred.

## `raveloxmidi_context_set_config`

```c
raveloxmidi_status_t raveloxmidi_context_set_config(
	raveloxmidi_context_t *context,
	const char *key,
	const char *value
);
```

Sets a configuration value on a context before it is started.

Arguments:

- `context`: Context returned by `raveloxmidi_context_create()`.
- `key`: Configuration key to set. This must not be `NULL`.
- `value`: Configuration value. `NULL` is accepted for keys that allow
  an unset value.

Return values:

- `RAVELOXMIDI_OK`: Configuration value was accepted.
- `RAVELOXMIDI_ERROR_INVALID_ARGUMENT`: `context` or `key` is `NULL`.
- `RAVELOXMIDI_ERROR_INVALID_STATE`: The context is already running.

Notes:

- Configuration must be applied before `raveloxmidi_context_start()`.
- `network.bind_address` must be set before startup.
- Current configuration keys mirror the existing raveloxmidi
  configuration names while the SDK configuration API is still forming.

## `raveloxmidi_context_get_config`

```c
raveloxmidi_status_t raveloxmidi_context_get_config(
	raveloxmidi_context_t *context,
	const char *key,
	const char **value
);
```

Gets a configuration value from a context.

Arguments:

- `context`: Context returned by `raveloxmidi_context_create()`.
- `key`: Configuration key to read. This must not be `NULL`.
- `value`: Output pointer that receives the configuration value. This
  must not be `NULL`.

Return values:

- `RAVELOXMIDI_OK`: Configuration value was found.
- `RAVELOXMIDI_ERROR_INVALID_ARGUMENT`: `context`, `key` or `value` is
  `NULL`.
- `RAVELOXMIDI_ERROR_NOT_FOUND`: No value is available for `key`.

Ownership:

- The returned value pointer is owned by the library.
- Callers must not modify or free the returned value.
- The pointer remains valid until the config item is changed or the
  context is freed.

## `raveloxmidi_context_set_config_file`

```c
raveloxmidi_status_t raveloxmidi_context_set_config_file(
	raveloxmidi_context_t *context,
	const char *filename
);
```

Sets and loads a configuration file for a context before it is started.

Arguments:

- `context`: Context returned by `raveloxmidi_context_create()`.
- `filename`: Path to the configuration file. This must not be `NULL`
  or empty.

Return values:

- `RAVELOXMIDI_OK`: The file was loaded and `config.file` was set.
- `RAVELOXMIDI_ERROR_INVALID_ARGUMENT`: `context` is `NULL`, or
  `filename` is `NULL` or empty.
- `RAVELOXMIDI_ERROR_INVALID_STATE`: The context is already running.
- `RAVELOXMIDI_ERROR`: The file could not be opened or loaded.

Notes:

- The file is loaded immediately.
- Values loaded from the file may be overridden by later calls to
  `raveloxmidi_context_set_config()`.
- `config.file` can be read with `raveloxmidi_context_get_config()`
  after this call succeeds.

## `raveloxmidi_context_dump_config`

```c
raveloxmidi_status_t raveloxmidi_context_dump_config( raveloxmidi_context_t *context );
```

Dumps the current configuration through the library logging system.

Arguments:

- `context`: Context returned by `raveloxmidi_context_create()`.

Return values:

- `RAVELOXMIDI_OK`: Configuration dump was requested.
- `RAVELOXMIDI_ERROR_INVALID_ARGUMENT`: `context` is `NULL`.

Notes:

- The current dump output is emitted at debug log level.
- If logging has not been initialized yet, this call initializes logging
  using the current configuration.

## `raveloxmidi_context_start`

```c
raveloxmidi_status_t raveloxmidi_context_start( raveloxmidi_context_t *context );
```

Starts the RTP-MIDI service for a context.

Startup initializes sockets, connection state, DNS service publishing,
the MIDI sender queue and, when enabled at build time, ALSA integration.
The network loop runs on a library-owned worker thread, so this call
returns after startup completes.

Arguments:

- `context`: Context returned by `raveloxmidi_context_create()`.

Return values:

- `RAVELOXMIDI_OK`: Service started.
- `RAVELOXMIDI_ERROR_INVALID_ARGUMENT`: `context` is `NULL`, or required
  configuration such as `network.bind_address` is missing.
- `RAVELOXMIDI_ERROR_INVALID_STATE`: The context is already running.
- `RAVELOXMIDI_ERROR`: A runtime startup step failed.

Notes:

- Callers should stop a successfully started context with
  `raveloxmidi_context_stop()`.
- If startup fails after partial initialization, the SDK attempts to
  unwind initialized runtime subsystems before returning.

## `raveloxmidi_context_request_stop`

```c
raveloxmidi_status_t raveloxmidi_context_request_stop( raveloxmidi_context_t *context );
```

Requests shutdown for a running context without waiting for the runtime
thread to finish.

Arguments:

- `context`: Context returned by `raveloxmidi_context_create()`.

Return values:

- `RAVELOXMIDI_OK`: Shutdown was requested.
- `RAVELOXMIDI_ERROR_INVALID_ARGUMENT`: `context` is `NULL`.
- `RAVELOXMIDI_ERROR_NOT_RUNNING`: The context has no running or
  partially initialized runtime subsystems to stop.

Notes:

- This is useful for signal handlers or event loops that need to ask the
  service to stop and then wait elsewhere.
- Call `raveloxmidi_context_wait()` after requesting shutdown to join
  the runtime thread and clean up runtime subsystems.

## `raveloxmidi_context_wait`

```c
raveloxmidi_status_t raveloxmidi_context_wait( raveloxmidi_context_t *context );
```

Waits for a running context to stop, joins the library-owned runtime
thread and tears down runtime subsystems.

Arguments:

- `context`: Context returned by `raveloxmidi_context_create()`.

Return values:

- `RAVELOXMIDI_OK`: Runtime stopped and cleanup completed.
- `RAVELOXMIDI_ERROR_INVALID_ARGUMENT`: `context` is `NULL`.
- `RAVELOXMIDI_ERROR_NOT_RUNNING`: The context has no running or
  partially initialized runtime subsystems to wait for.

Notes:

- This call blocks until shutdown is requested. Shutdown can be requested
  by `raveloxmidi_context_request_stop()`, by `raveloxmidi_context_stop()`
  or by the runtime receiving its local shutdown command.

## `raveloxmidi_context_stop`

```c
raveloxmidi_status_t raveloxmidi_context_stop( raveloxmidi_context_t *context );
```

Requests shutdown for a running context, waits for the runtime thread to
finish and tears down runtime subsystems created by
`raveloxmidi_context_start()`.

Arguments:

- `context`: Context returned by `raveloxmidi_context_create()`.

Return values:

- `RAVELOXMIDI_OK`: Runtime subsystems were stopped.
- `RAVELOXMIDI_ERROR_INVALID_ARGUMENT`: `context` is `NULL`.
- `RAVELOXMIDI_ERROR_NOT_RUNNING`: The context has no running or
  partially initialized runtime subsystems to stop.

Notes:

- `raveloxmidi_context_stop()` is equivalent to calling
  `raveloxmidi_context_request_stop()` followed by
  `raveloxmidi_context_wait()`.
- It is valid to call `raveloxmidi_context_free()` without first calling
  `raveloxmidi_context_stop()`; free will stop a running context.

## `raveloxmidi_context_free`

```c
void raveloxmidi_context_free( raveloxmidi_context_t **context );
```

Frees a context created by `raveloxmidi_context_create()`.

Arguments:

- `context`: Address of a context pointer. If `context` or `*context` is
  `NULL`, the function returns without doing anything.

Ownership:

- If the context is running, it is stopped before being freed.
- The context pointer is set to `NULL` before returning.
- After this call, the caller must not use the old context pointer.

## Public Event Types

The SDK currently defines `raveloxmidi_event_type_t` for future MIDI
event callback APIs:

- `RAVELOXMIDI_EVENT_NOTE_OFF`
- `RAVELOXMIDI_EVENT_NOTE_ON`
- `RAVELOXMIDI_EVENT_CONTROL_CHANGE`
- `RAVELOXMIDI_EVENT_PROGRAM_CHANGE`
- `RAVELOXMIDI_EVENT_RAW_MIDI`

Callback registration and event delivery APIs are planned in Phase 3
and are not available yet.
