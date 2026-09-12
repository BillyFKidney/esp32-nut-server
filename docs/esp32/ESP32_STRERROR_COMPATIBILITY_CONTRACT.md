# `strerror()` fallback compatibility contract

## Scope

`src/common/strerror.c` is inherited compatibility code used only when the
target libc does not provide `strerror()`. It is not part of the normal ESP32
firmware path, where libc supplies the function.

## Preserved behavior

- Compile the fallback only when `HAVE_STRERROR` is absent.
- Return the established text for available errno macros.
- Prefer `EWOULDBLOCK` before `EAGAIN` when the values alias; retain the
  distinct `EAGAIN` mapping only when their numeric values differ.
- Return `Error <number>` for an unrecognized errno value.
- Retain the existing static fallback-buffer lifetime for unrecognized values.

## Fixture and release gate

Run `tests/run-strerror-fallback-test.sh` on a host toolchain. The fixture
forces the fallback by compiling `strerror.c` without `HAVE_STRERROR` and
renaming its symbol, so it never replaces the host libc implementation.

Any future structural change requires this fixture plus a documented target
matrix for the supported portability configurations. A firmware release that
touches this file also requires the ordinary exact-tag ESP32 build and
3Dprinter OTA/service regression acceptance; that device check verifies the
normal libc path, not the forced fallback path.
