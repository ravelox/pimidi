# Codex Optimization Review

This document captures optimization recommendations from a read-only review of the
`pimidi` / `raveloxmidi` codebase, along with implementation status as changes
are made on the `ai-optimize` branch.

## Summary

The main performance opportunities are in allocation-heavy MIDI packet assembly,
queue lock contention, repeated log file open/close calls, and repeated network
address setup during send. The project is relatively small and the default
connection count is low, so the best improvements are likely to come from reducing
latency and jitter in hot paths rather than broad architectural rewrites.

Recommended order:

1. Reduce lock hold time in the MIDI sender queue. Completed.
2. Reduce allocation/copy churn in RTP-MIDI packet packing. Completed.
3. Keep the log file open between log calls. Completed.
4. Cache per-connection socket address data. Completed.
5. Replace `select()` descriptor scanning with `poll()`. Completed.

## Implemented Changes

### Queue Handler Lock Scope

Status: completed.

`data_queue_handler()` now removes a queue item while holding `queue->lock`, then
copies the action pointer and item payload/context before unlocking. The handler
callback runs after the queue lock is released, and the queue item is freed after
the callback returns.

Validation:

- `git diff --check`
- `gcc -fsyntax-only -I raveloxmidi/include raveloxmidi/src/data_queue.c`

### RTP-MIDI Send Buffer Packing

Status: completed.

`midi_sender_send_single()` now computes the final RTP packet size after journal
packing, allocates one RTP packet buffer, and writes the RTP header, MIDI payload
header, MIDI payload data, and optional journal directly into that buffer.

This removes the previous intermediate combined RTP payload buffer, avoids
allocating `rtp_packet->payload`, avoids the `rtp_packet_pack()` header allocation
plus `realloc()` path for outbound MIDI sends, and keeps per-destination packed
buffers scoped to each loop iteration.

Validation:

- `git diff --check`
- `gcc -fsyntax-only -I /tmp/pimidi-codex-include -I raveloxmidi/include raveloxmidi/src/midi_sender.c`

### Persistent Log File Handle

Status: completed.

`logging_init()` now opens the configured log file once and stores the stream for
reuse by `logging_printf()`. `logging_printf()` writes through the cached stream,
falling back to `stderr` when no log file is configured or opening the configured
file fails. File-backed logs are flushed after each message to preserve the old
close-after-each-write visibility behavior without repeated open/close overhead.
`logging_teardown()` closes the cached file stream.

Validation:

- `git diff --check`
- `gcc -fsyntax-only -I /tmp/pimidi-codex-include -I raveloxmidi/include raveloxmidi/src/logging.c`

### Cached Connection Socket Addresses

Status: completed.

`net_ctx_t` now stores cached control and data destination socket addresses plus
their lengths. `net_ctx_set()` populates those cached addresses when a connection
is registered or reused, and `net_ctx_reset()` clears them. `net_ctx_send()` now
selects the cached control or data address instead of calling `get_sock_info()`
for every packet.

Validation:

- `git diff --check`
- `gcc -fsyntax-only -I /tmp/pimidi-codex-include -I raveloxmidi/include raveloxmidi/src/net_connection.c`

### Network Socket Poll Loop

Status: completed.

`net_socket_fd_loop()` now uses a `pollfd` table for the network sockets and the
shutdown pipe. `net_socket_set_fds()` rebuilds the table from active socket
entries without scanning every descriptor up to `max_fd`, and the event loop
handles only descriptors returned by `poll()`.

Scope:

- Applies only to `raveloxmidi/src/net_socket.c`.
- Does not create a shared polling abstraction at this time.
- Leaves ALSA polling in `raveloxmidi_alsa.c` unchanged.
- Leaves remote connection sync polling unchanged.

Validation:

- `git diff --check`
- `gcc -fsyntax-only -I /tmp/pimidi-codex-include -I raveloxmidi/include raveloxmidi/src/net_socket.c`

## High-Impact Recommendations

### 1. Process Queue Items Outside The Queue Lock

Location: `raveloxmidi/src/data_queue.c`

`data_queue_handler()` holds `queue->lock` while it calls `queue->action()`.
For the MIDI sender queue, that action can allocate memory, pack RTP payloads,
write sockets, write ALSA output, and log. Holding the queue mutex for that whole
operation blocks producers from enqueueing new MIDI events.

Relevant code:

- `data_queue_handler()` locks at approximately line 254.
- `queue->action(item->data, item->context)` is called while still locked at
  approximately line 276.

Recommendation:

- Pop the queue item while holding the mutex.
- Unlock immediately after removing the item.
- Process `queue->action()` outside the mutex.
- Re-lock only if queue state must be updated afterward.

Expected benefit:

- Lower producer contention.
- Lower MIDI event latency under bursts.
- Less risk that slow network/ALSA/log operations stall enqueueing.

### 2. Reduce Heap Allocation And Copies In The MIDI Send Path

Location: `raveloxmidi/src/midi_sender.c`

`midi_sender_send_single()` allocates and copies several buffers per connection
per MIDI command:

- MIDI payload buffer.
- Packed journal buffer.
- Combined RTP payload buffer.
- `rtp_packet->payload`.
- Final packed RTP packet buffer.

Relevant code:

- Per-connection loop starts around line 175.
- Payload is repacked around line 205.
- Combined payload is allocated and copied around lines 209-224.
- Final RTP packet is allocated by `rtp_packet_pack()` around line 228.

Recommendation:

- Compute the final RTP packet size once per destination.
- Pack directly into a single pre-sized buffer.
- Avoid copying through both `packed_rtp_payload` and `rtp_packet->payload`.
- Consider a reusable per-thread scratch buffer if packet sizes remain bounded.

Expected benefit:

- Less allocator pressure.
- Fewer memory copies per MIDI event.
- Lower jitter during dense note/control streams.

### 3. Keep The Log File Open

Location: `raveloxmidi/src/logging.c`

`logging_printf()` opens the configured log file for every message and closes it
before returning.

Relevant code:

- `fopen(logging_file_name, "a+")` happens around line 134.
- `fclose(logging_fp)` happens around line 161.

Recommendation:

- Open the log file once in `logging_init()`.
- Store the `FILE *` globally.
- Flush periodically or after each message if immediate durability is preferred.
- Close the file in `logging_teardown()`.

Expected benefit:

- Major reduction in syscall overhead when logging is enabled.
- Smoother performance in debug mode.
- Less lock hold time inside `logging_printf()`.

### 4. Cache Destination Socket Addresses Per Connection

Location: `raveloxmidi/src/net_connection.c`

`net_ctx_send()` rebuilds the destination address with `get_sock_info()` every
time it sends a packet.

Relevant code:

- Destination address setup happens around lines 547-550.
- `sendto()` happens around line 554.

Recommendation:

- Store precomputed control and data `sockaddr_storage` plus lengths in
  `net_ctx_t`.
- Populate these fields when a connection is registered or updated.
- Let `net_ctx_send()` select the cached control/data address and call `sendto()`
  directly.

Expected benefit:

- Less per-packet CPU work.
- Less parsing/lookup overhead in the send path.
- Cleaner send code.

### 5. Replace `select()` Descriptor Scanning With `poll()`

Location: `raveloxmidi/src/net_socket.c`

The socket loop rebuilds fd sets each iteration, calls `select()`, then scans
every fd from zero to `max_fd`.

Relevant code:

- `net_socket_set_fds()` is called around line 727.
- `select(max_fd + 1, ...)` is called around line 733.
- The loop scans `0..max_fd` around lines 738-750.

Recommendation:

- Use a `pollfd` array for network sockets, similar to the ALSA path.
- Track only active descriptors.

Implementation scope:

- Replace only the network socket `select()` loop in `net_socket_fd_loop()`.
- Build a local `pollfd` table in `net_socket_set_fds()` from the active socket
  table entries.
- Include the shutdown pipe read fd in the network `pollfd` table so shutdown can
  wake the network loop promptly.
- Keep ALSA polling and remote connection sync polling separate for now.
- Do not introduce a single polling abstraction until the behavior of the
  independent polling loops is better covered by runtime tests.

Expected benefit:

- Less unnecessary fd scanning.
- More scalable descriptor handling.
- Simpler shutdown descriptor integration.

## Correctness Issues With Performance Impact

### C1. `dbuffer_write()` Appears To Reallocate On Every Write

Location: `raveloxmidi/src/dbuffer.c`

The condition:

```c
if( dbuffer->len < ( dbuffer->len + in_buffer_len + 1) )
```

is true for any nonzero `in_buffer_len`, so `dbuffer_write()` appears to grow the
buffer on every write rather than only when capacity is insufficient.

Relevant code:

- Around lines 122-128.

Recommendation:

- Compare required length against allocated capacity, likely represented by
  `num_blocks * block_size`.
- Return the number of bytes written; the function currently returns `0` even
  after successful writes.

### C2. Bounds Checks Use `>` Instead Of `>=`

Location: `raveloxmidi/src/data_table.c`

Several index checks use `index > table->count`, which allows
`index == table->count` through even though that is out of bounds.

Relevant code:

- `data_table_item_get()` around line 274.
- Similar checks appear in nearby table helpers.

Recommendation:

- Change these checks to `index >= table->count`.
- Add focused tests or assertions for empty, last valid, and first invalid index.

### C3. Allocation Check Happens After `memset()`

Location: `raveloxmidi/src/rtp_packet.c`

`rtp_packet_pack()` calls `memset()` on `*out_buffer` before checking whether the
allocation succeeded.

Relevant code:

- Allocation and `memset()` happen around lines 89-92.

Recommendation:

- Check `*out_buffer` immediately after allocation and before `memset()`.

### C4. Fixed-Size JSON Formatting Could Overflow

Location: `raveloxmidi/src/net_connection.c`

`net_ctx_connections_to_string()` uses `sprintf()` into a fixed 1024-byte local
buffer while formatting connection data that includes names and host strings.

Relevant code:

- `sprintf()` calls around lines 611, 624, and 637.

Recommendation:

- Use `snprintf()` at minimum.
- Prefer appending directly to `dstring_t` with bounded formatting helpers.
- Escape JSON string values if connection names can contain quotes or control
  characters.

## Lower-Priority Opportunities

### Avoid Allocation In Ring Buffer Reads Where Possible

Location: `raveloxmidi/src/ring_buffer.c`

`ring_buffer_read()` always allocates a new destination buffer. Some callers only
need to inspect or advance a few bytes.

Recommendation:

- Add APIs for peeking bytes or reading into a caller-provided buffer.
- Use those APIs in parser paths that currently allocate only to compare or
  advance data.

### Gate Debug Dumps Early

Locations: multiple files, including `midi_sender.c`, `net_connection.c`,
`ring_buffer.c`, and `net_socket.c`

Many debug dump calls are already protected by `DEBUG_ONLY` inside the dump
function, but callers still make the function call and sometimes compute values
or allocate buffers first.

Recommendation:

- Keep the current threshold check in `logging_printf()`.
- For very hot paths, consider explicit `logging_get_threshold()` checks before
  expensive dump preparation.

### Add A Small Benchmark Or Load Test Harness

The repository includes Python send scripts and debug notes for sanitizers and
Valgrind, but no automated performance harness.

Recommendation:

- Add a repeatable local benchmark that sends a fixed number of NoteOn/NoteOff
  and Control Change events.
- Capture throughput, average latency if practical, and max/percentile latency.
- Run before and after changes to verify that optimizations reduce jitter rather
  than just moving work around.

## Suggested First Patch Set

For a low-risk first optimization pass:

1. Completed: move `queue->action()` outside `data_queue_handler()`'s lock.
2. Fix C1: `dbuffer_write()` capacity comparison and return value.
3. Fix C2: `data_table.c` bounds checks from `>` to `>=`.
4. Fix C3: move the allocation check before `memset()` in `rtp_packet_pack()`.
5. Fix C4: switch `net_ctx_connections_to_string()` from `sprintf()` to `snprintf()`.

These changes are small, easy to review, and improve either latency or safety
without changing the public behavior of the daemon.
