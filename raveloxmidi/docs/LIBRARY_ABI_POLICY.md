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

Internal headers and internal functions are not SDK surface, even while
some of those symbols remain visible for the current `raveloxmidi`
binary.

## Hidden Visibility Enforcement

The build intentionally does not yet compile the library with
`-fvisibility=hidden` or a linker export map. `P2-007` is still open,
and the `raveloxmidi` binary still links against internal library
symbols. Enforcing hidden visibility before the binary moves to the
public SDK API would break that link.

After `P2-007` is complete, the next visibility step should be:

- Add compiler detection for `-fvisibility=hidden`.
- Compile `libraveloxmidi` with hidden visibility by default.
- Keep only `RAVELOXMIDI_API` symbols exported.
- Verify exported symbols with `nm -D` or an equivalent platform tool.

## Libtool Versioning

`libraveloxmidi` uses libtool `-version-info current:revision:age`.
The current values are declared in `raveloxmidi/src/Makefile.am`:

```make
LIBRAVELOXMIDI_LT_CURRENT = 1
LIBRAVELOXMIDI_LT_REVISION = 0
LIBRAVELOXMIDI_LT_AGE = 1
```

The library is at ABI interface `1` while the SDK is still being formed
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
