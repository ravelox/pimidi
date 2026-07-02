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

- [ ] **CPPCHECK-006 unsignedLessThanZero**: `raveloxmidi/src/ring_buffer.c:102` checks unsigned `size` against zero with `size <= 0`.
- [ ] **CPPCHECK-007 unsignedLessThanZero**: `raveloxmidi/src/net_connection.c:551` checks unsigned `buffer_len` against zero.
- [ ] **CPPCHECK-008 unsignedLessThanZero**: `raveloxmidi/src/raveloxmidi_alsa.c:393` checks unsigned `buffer_size` against zero.
- [ ] **CPPCHECK-009 unsignedLessThanZero**: `raveloxmidi/src/utils.c:221` checks unsigned `len` against zero.

## Redundant Pointer Operations

- [x] **CPPCHECK-010 redundantPointerOp**: `raveloxmidi/src/chapter_c.c:197` uses redundant `&(*chapter_c)`. Addressed by passing `chapter_c` directly to `X_FREENULL()`.
- [x] **CPPCHECK-011 redundantPointerOp**: `raveloxmidi/src/midi_payload.c:344` uses redundant pointer operation on `payload`. Addressed by passing `payload` directly to `midi_payload_destroy()`.

## Unread Variables

- [ ] **CPPCHECK-012 unreadVariable**: `raveloxmidi/src/chapter_n.c:182` variable `i` is assigned but never used.
- [ ] **CPPCHECK-013 unreadVariable**: `raveloxmidi/src/daemon.c:107` variable `ret` is assigned but never used.
- [ ] **CPPCHECK-014 unreadVariable**: `raveloxmidi/src/data_queue.c:240` variable `action` is assigned but never used.
- [ ] **CPPCHECK-015 unreadVariable**: `raveloxmidi/src/data_table.c:43` variable `index` is assigned but never used.
- [ ] **CPPCHECK-016 unreadVariable**: `raveloxmidi/src/data_table.c:44` variable `count` is assigned but never used.
- [x] **CPPCHECK-017 unreadVariable**: `raveloxmidi/src/data_table.c:149` variable `i` is assigned but never used. Addressed by removing the redundant initializer before the `for` loop assigns `i`.
- [ ] **CPPCHECK-018 unreadVariable**: `raveloxmidi/src/midi_journal.c:205` variable `p` is assigned but never used.
- [ ] **CPPCHECK-019 unreadVariable**: `raveloxmidi/src/midi_sender.c:342` variable `bytes_written` is assigned but never used.
- [ ] **CPPCHECK-020 unreadVariable**: `raveloxmidi/src/midi_state.c:249` variable `read_status` is assigned but never used.
- [ ] **CPPCHECK-021 unreadVariable**: `raveloxmidi/src/net_socket.c:243` variable `ring_buffer_size` is assigned but never used.
- [ ] **CPPCHECK-022 unreadVariable**: `raveloxmidi/src/net_socket.c:752` variable `timeout` is assigned but never used.
- [ ] **CPPCHECK-023 unreadVariable**: `raveloxmidi/src/net_socket.c:753` variable `i` is assigned but never used.
- [ ] **CPPCHECK-024 unreadVariable**: `raveloxmidi/src/raveloxmidi_alsa.c:576` variable `i` is assigned but never used.
- [ ] **CPPCHECK-025 unreadVariable**: `raveloxmidi/src/raveloxmidi_alsa.c:610` variable `poll_result` is assigned but never used.
- [ ] **CPPCHECK-026 unreadVariable**: `raveloxmidi/src/raveloxmidi_alsa.c:611` variable `i` is assigned but never used.
- [ ] **CPPCHECK-027 unreadVariable**: `raveloxmidi/src/utils.c:210` variable `c` is assigned but never used.

## Bulk Style Buckets

- [ ] **CPPCHECK-028 constParameterPointer**: 28 parameters can be declared as pointers to const.
- [ ] **CPPCHECK-029 constVariablePointer**: 17 variables can be declared as pointers to const.
- [ ] **CPPCHECK-030 variableScope**: 10 variables can have narrower scope.
- [ ] **CPPCHECK-031 unusedFunction**: 43 functions are reported unused.
- [ ] **CPPCHECK-032 staticFunction**: 96 functions are reported as candidates for internal linkage.

## Run Configuration Noise

- [x] **CPPCHECK-033 missingInclude**: 245 project include lookup warnings. Addressed by updating the cppcheck command to run after `./configure` and pass `-I include -I .`.
- [x] **CPPCHECK-034 missingIncludeSystem**: 208 system include lookup warnings. Addressed by adding `--suppress=missingIncludeSystem` to the cppcheck command.
- [x] **CPPCHECK-035 normalCheckLevelMaxBranches**: 19 branch-limit notices. Addressed by adding `--check-level=exhaustive` to the cppcheck command.

## Addressed

- [x] **CPPCHECK-036 missingReturn**: `raveloxmidi/src/ring_buffer.c:437` reported missing return in unused `ring_buffer_char()`. Addressed by commit `447a191` removing the unused stub.
