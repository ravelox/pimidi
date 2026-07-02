# Cppcheck Tracking

This file tracks findings from `cppcheck.results` so each item can be marked
off as it is reviewed or fixed. The original result file is generated output and
may be stale; this checklist is the durable record.

## Status Legend

- `[ ]` Open
- `[x]` Addressed
- `[~]` Reviewed; no code change planned

## High-Signal Findings

- [x] **CPPCHECK-001 resourceLeak**: `raveloxmidi/src/daemon.c:96` reports a resource leak for `reopen_file` after `freopen("/dev/null", "w", stdout)`. Addressed by checking `freopen()` inline instead of reusing `reopen_file`.
- [x] **CPPCHECK-002 resourceLeak**: `raveloxmidi/src/daemon.c:101` reports a second resource leak in the same daemon stdio redirection path. Addressed by the same inline `freopen()` check.
- [x] **CPPCHECK-003 nullPointerOutOfMemory**: `raveloxmidi/src/utils.c:480` may dereference `utils_timestamp_string` if `malloc(100)` fails in `utils_timestamp()`. Addressed by checking allocation, returning a fallback timestamp string, and using bounded formatting.
- [x] **CPPCHECK-004 knownConditionTrueFalse**: `raveloxmidi/src/raveloxmidi_alsa.c:298` reports `!rawmidi` is always false. Addressed by removing the redundant null check after `data` has already been checked.
- [x] **CPPCHECK-005 redundantCondition**: `raveloxmidi/src/raveloxmidi_config.c:117` reports `*p1` is redundant when checking `*p1 == '#'`. Addressed by simplifying the comment check to `*p1 == '#'`.

## Unsigned Comparisons

- [x] **CPPCHECK-006 unsignedLessThanZero**: `raveloxmidi/src/ring_buffer.c:102` checks unsigned `size` against zero with `size <= 0`. Addressed by using `size == 0`.
- [x] **CPPCHECK-007 unsignedLessThanZero**: `raveloxmidi/src/net_connection.c:551` checks unsigned `buffer_len` against zero. Addressed by using `buffer_len == 0`.
- [x] **CPPCHECK-008 unsignedLessThanZero**: `raveloxmidi/src/raveloxmidi_alsa.c:392` checks unsigned `buffer_size` against zero. Addressed by using `buffer_size == 0`.
- [x] **CPPCHECK-009 unsignedLessThanZero**: `raveloxmidi/src/utils.c:220` checks unsigned `len` against zero. Addressed by using `len == 0`.
- [x] **CPPCHECK-037 unsignedLessThanZero**: `raveloxmidi/src/kv_table.c:98` checks unsigned `(*table)->count` against zero. Addressed by using `(*table)->count == 0`.
- [x] **CPPCHECK-038 unsignedLessThanZero**: `raveloxmidi/src/kv_table.c:142` checks unsigned `table->count` against zero. Addressed by using `table->count == 0`.

## Redundant Pointer Operations

- [x] **CPPCHECK-010 redundantPointerOp**: `raveloxmidi/src/chapter_c.c:197` uses redundant `&(*chapter_c)`. Addressed by passing `chapter_c` directly to `X_FREENULL()`.
- [x] **CPPCHECK-011 redundantPointerOp**: `raveloxmidi/src/midi_payload.c:344` uses redundant pointer operation on `payload`. Addressed by passing `payload` directly to `midi_payload_destroy()`.

## Unread Variables

- [x] **CPPCHECK-012 unreadVariable**: `raveloxmidi/src/chapter_n.c:182` variable `i` is assigned but never used. Addressed by moving `i` into the block where it initializes `chapter_n->notes`.
- [x] **CPPCHECK-013 unreadVariable**: `raveloxmidi/src/daemon.c:107` variable `ret` is assigned but never used. Addressed by checking `stat()` and `unlink()` results inline.
- [x] **CPPCHECK-014 unreadVariable**: `raveloxmidi/src/data_queue.c:240` variable `action` is assigned but never used. No longer reported by the fresh cppcheck run with the documented invocation.
- [x] **CPPCHECK-015 unreadVariable**: `raveloxmidi/src/data_table.c:43` variable `index` is assigned but never used. Addressed by moving `index` into the debug dump block where it is used.
- [x] **CPPCHECK-016 unreadVariable**: `raveloxmidi/src/data_table.c:44` variable `count` is assigned but never used. Addressed by initializing `count` inside the debug dump block where it is used.
- [x] **CPPCHECK-017 unreadVariable**: `raveloxmidi/src/data_table.c:149` variable `i` is assigned but never used. Addressed by removing the redundant initializer before the `for` loop assigns `i`.
- [x] **CPPCHECK-018 unreadVariable**: `raveloxmidi/src/midi_journal.c:205` variable `p` is assigned but never used. Addressed by removing the final pointer advance after the last copied chapter.
- [x] **CPPCHECK-019 unreadVariable**: `raveloxmidi/src/midi_sender.c:342` variable `bytes_written` is assigned but never used. Addressed by moving `bytes_written` into the `inbound_midi_fd` block and using `ssize_t` for the `write()` result.
- [x] **CPPCHECK-020 unreadVariable**: `raveloxmidi/src/midi_state.c:249` variable `read_status` is assigned but never used. Addressed by moving `read_status` into the read loop without a redundant initializer.
- [x] **CPPCHECK-021 unreadVariable**: `raveloxmidi/src/net_socket.c:243` variable `ring_buffer_size` is assigned but never used. Addressed by moving `ring_buffer_size` into the socket-buffer allocation success branch where it is assigned and used.
- [x] **CPPCHECK-022 unreadVariable**: `raveloxmidi/src/net_socket.c:752` variable `timeout` is assigned but never used. Addressed by moving `timeout` into the polling loop where it is assigned and used.
- [x] **CPPCHECK-023 unreadVariable**: `raveloxmidi/src/net_socket.c:753` variable `i` is assigned but never used. Addressed by moving `i` into the polling result block.
- [x] **CPPCHECK-024 unreadVariable**: `raveloxmidi/src/raveloxmidi_alsa.c:576` variable `i` is assigned but never used. Addressed by moving `i` into the poll-descriptor loop block.
- [x] **CPPCHECK-025 unreadVariable**: `raveloxmidi/src/raveloxmidi_alsa.c:610` variable `poll_result` is assigned but never used. Addressed by moving `poll_result` into the listener polling loop.
- [x] **CPPCHECK-026 unreadVariable**: `raveloxmidi/src/raveloxmidi_alsa.c:611` variable `i` is assigned but never used. Addressed by moving `i` into the descriptor-processing block.
- [x] **CPPCHECK-027 unreadVariable**: `raveloxmidi/src/utils.c:210` variable `c` is assigned but never used. Addressed by moving `c` into the hex dump loop.

## Bulk Style Buckets

- [x] **CPPCHECK-028 constParameterPointer**: 35 parameters can be declared as pointers to const. Addressed by adding `const` qualifiers to read-only pointer parameters in the affected headers and implementations.
- [ ] **CPPCHECK-029 constVariablePointer**: 22 variables can be declared as pointers to const.
- [x] **CPPCHECK-030 variableScope**: 10 variables can have narrower scope. Addressed by narrowing the remaining reported variable declarations; the fresh cppcheck run reports no `variableScope` findings.
- [~] **CPPCHECK-031 unusedFunction**: 35 functions are reported unused. Reviewed as intentional public library/API surface; suppress `unusedFunction` in cppcheck instead of removing exported functions.
- [~] **CPPCHECK-032 staticFunction**: 96 functions are reported as candidates for internal linkage. Reviewed as intentional public library/API surface; suppress `staticFunction` in cppcheck instead of changing linkage.

## Run Configuration Noise

- [x] **CPPCHECK-033 missingInclude**: 245 project include lookup warnings. Addressed by updating the cppcheck command to run after `./configure` and pass `-I include -I .`.
- [x] **CPPCHECK-034 missingIncludeSystem**: 208 system include lookup warnings. Addressed by adding `--suppress=missingIncludeSystem` to the cppcheck command.
- [x] **CPPCHECK-035 normalCheckLevelMaxBranches**: 19 branch-limit notices. Addressed by adding `--check-level=exhaustive` to the cppcheck command.

## Addressed

- [x] **CPPCHECK-036 missingReturn**: `raveloxmidi/src/ring_buffer.c:437` reported missing return in unused `ring_buffer_char()`. Addressed by commit `447a191` removing the unused stub.
