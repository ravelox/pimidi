# Library ABI Policy

This document records the symbol visibility and libtool versioning
policy for `libraveloxmidi`. It is the completion record for `P2-009`
and `P2-010` in `LIBRARY_REFACTOR.md`.

## Public Symbols

Public SDK symbols are declarations in `raveloxmidi.h` marked with
`RAVELOXMIDI_API`.

The `RAVELOXMIDI_API` macro expands to:

- `__declspec(dllexport)` while building the library on Windows-like
  targets.
- `__declspec(dllimport)` for Windows-like SDK consumers.
- `__attribute__((visibility("default")))` for GCC-compatible
  compilers.
- Nothing on compilers without a supported visibility mechanism.

Internal headers and internal functions are not SDK surface, even if
some of those symbols remain visible until hidden visibility is enforced.

## Hidden Visibility Enforcement

The `raveloxmidi` binary links only against public SDK symbols. The build
checks whether the compiler supports `-fvisibility=hidden` and, when it
does, compiles `libraveloxmidi` with hidden visibility by default.

Public SDK declarations remain exported through `RAVELOXMIDI_API`.
Internal implementation symbols should not appear in the dynamic symbol
table on platforms where hidden visibility is supported.

Verify exported symbols with:

```sh
nm -D --defined-only src/.libs/libraveloxmidi.so | awk '{print $3}' | grep '^raveloxmidi'
```

Only the `raveloxmidi_` public SDK functions documented in
`PUBLIC_API.md` should be listed.

## Libtool Versioning

`libraveloxmidi` uses libtool `-version-info current:revision:age`.
The current values are declared in `raveloxmidi/src/Makefile.am`:

```make
LIBRAVELOXMIDI_LT_CURRENT = 2
LIBRAVELOXMIDI_LT_REVISION = 1
LIBRAVELOXMIDI_LT_AGE = 2
```

The library is at ABI interface `2` while the SDK is still being formed
on the `library` development branch.

Use libtool's standard update rules when changing the public ABI:

- If the library source changes without changing the public ABI,
  increment `revision`.
- If public ABI symbols are added without removing or changing existing
  ABI, increment `current`, increment `age`, and reset `revision` to
  `0`.
- If public ABI symbols are removed or changed incompatibly, increment
  `current`, reset `age` to `0`, and reset `revision` to `0`.

Public ABI means exported `RAVELOXMIDI_API` functions, public types,
public enum values, public struct layouts if any are introduced, and the
documented ownership/behavior contract of those APIs.
