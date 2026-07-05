# Review Pass Findings

## Findings

- **RP-001 High**: `raveloxmidi/src/data_table.c:293` can deadlock and then invalid-free in `data_table_delete_item()`. It locks `table`, locks it again inside the destructor branch, then calls `X_FREE(&(table->items[index]->data))`, which passes the address of the field rather than the allocated pointer. Also, when `item_destructor` is `NULL`, the item is never marked unused.

- **RP-002 High**: `raveloxmidi/src/rtp_packet.c:124` trusts packet length while unpacking network data. Short UDP packets can drive `get_uint16()` or `get_uint32()` past the buffer, and the payload copy path does not check whether `X_MALLOC()` succeeded.

- **RP-003 Medium**: `raveloxmidi/src/net_applemidi.c:419` assigns `X_REALLOC()` directly back to `*out_buffer` and immediately uses it in several branches. If realloc fails, the original buffer is leaked and later pointer arithmetic can dereference `NULL`. Use a temporary pointer and check before updating `*out_buffer`.

- **RP-004 Medium**: `raveloxmidi/src/dbuffer.c:172` calls `memset(*out_buffer, ...)` before checking whether `X_MALLOC()` returned `NULL`. Under memory pressure this becomes an immediate crash.

- **RP-005 Medium - Fixed**: `raveloxmidi/src/net_socket.c` now keeps the shutdown request path signal-safe by setting a `sig_atomic_t` flag and writing only to the shutdown pipe write end. Logging, mutex use and descriptor close operations were removed from the shutdown request path; pipe descriptors are closed during normal loop teardown.

- **RP-006 Medium**: `raveloxmidi/src/net_applemidi.c:276` treats the remaining INV name bytes as a trusted C string via `X_STRDUP((const char *)p)`. A malformed packet without a NUL terminator can read past the packet buffer. Use bounded duplication from `in_buffer_len`.

- **RP-007 Low**: `raveloxmidi/configure.ac:43` calls `AC_OUTPUT` inside the `dpkg` conditional and again later. Configure completed in a throwaway source build, but it runs `config.status` twice and is brittle, especially for out-of-tree builds.

- **RP-008 Low - Fixed**: Python helpers now compile under Python 3. `python/new_drum.py` and `python/send100.py` were updated from Python 2 print syntax, and `python/new_drum.py` now imports `sys`.

- **RP-009 Low - Fixed**: `python/send_status.py`, `python/send_list.py`, and `python/send_quit.py` now use a bounded socket timeout and exit nonzero if the daemon does not respond.

## Validation

- `git diff --check`
- Full throwaway C build: `./autogen.sh && ./configure && make -j2`
- Python compilation: `python3 -m py_compile python/*.py`

The C build succeeds, with warnings for ignored `read()` and `write()` return values in `net_socket.c`. Python compilation now passes for all helper scripts. `cppcheck` is not installed in this environment.
