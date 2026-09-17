# v2.9.2 staged-PSRAM OTA contract

## Goal

An ADMIN browser upload remains exclusively in PSRAM while it is received and
checked. Neither OTA application partition may be erased, written, selected,
or otherwise changed by a failed, abandoned, malformed, oversized, timed-out,
or merely checked upload. Only an explicit **Install Firmware** action may
begin writing the inactive application partition.

This is intentionally a new contract. It does not revise the historical
v2.8.4 route-preservation contract.

## Browser routes

- `POST /api/v1/ota/check` remains ADMIN-session plus CSRF protected and
  requires exactly `Content-Type: application/octet-stream`. It accepts one
  complete image no larger than the inactive application partition, allocates
  its staging buffer only with `MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT`, and
  checks the complete in-memory ESP image before returning `checked`.
- A successful response supplies the checked firmware version and an opaque,
  unpredictable stage identifier. It does not disclose memory addresses or
  secret material.
- `POST /api/v1/ota/install` remains ADMIN-session plus CSRF protected. It
  has no firmware body; it supplies the stage identifier in one dedicated
  request header. A missing, expired, replaced, malformed, or mismatched
  identifier is rejected before any OTA partition work.
- Install rechecks the retained image, copies it to the inactive partition in
  bounded chunks, then uses ESP-IDF OTA final validation before boot selection.
  Only that successful sequence records `pending` and schedules a restart.

## Image validation

ESP-IDF v6.0.2's public `esp_image_verify()` reads only flash partitions.
The PSRAM check must therefore mirror the active image-format checks for this
target: header/chip compatibility, bounded aligned segments, application
description, ROM checksum, and appended SHA-256. Secure Boot and anti-rollback
are disabled in this configured product; if either becomes enabled, staging
must reject the configuration rather than claim equivalent pre-write
verification. ESP-IDF `esp_ota_end()` remains the authoritative final check
after the explicit Install action.

The check preserves both slots through the entire upload and verification
phase. During an explicit install, the inactive slot must necessarily be
erased and rewritten; a third persistent staging location would be required to
keep both application slots valid during that physical write.

## Staging lifetime and synchronization

- Retain one checked image for at most ten minutes. Expiry zeroizes and frees
  the PSRAM allocation. Reboot always loses staging without flash mutation.
- A complete successful check atomically replaces the old staged image. Failed
  replacement leaves a previously checked image intact until its own expiry.
- Check, replacement, expiry, direct Agent install, and browser install are
  serialized. A conflicting operation receives a bounded busy/conflict result.
- Install success or failure consumes and zeroizes the staging allocation.
  Browser reconnect, refresh, logout, and session expiry do not install it.
- PSRAM allocation pressure rejects the check; it never falls back to internal
  heap or flash storage.

## Agent route

`POST /api/v1/agent/ota/install` remains the scoped `ota.install` direct
install route. It cannot access a browser stage, and diagnostic tokens retain
no OTA authority. Its temporary receive buffer requests PSRAM first, with the
existing install/rollback behavior preserved.

## Required acceptance

- Valid check returns a stage identifier/version while application partition
  bytes and boot selection remain unchanged.
- Malformed, truncated, oversized, timed-out, interrupted, and PSRAM-pressure
  checks leave both application slots untouched.
- Missing, expired, replaced, and wrong stage identifiers produce no writes.
- Only a valid explicit install writes the inactive partition; successful
  install passes ESP-IDF final validation, selects it, restarts, and preserves
  rollback validity.
- ADMIN/CSRF, bearer scope, content-type, concurrency, expiry, replacement,
  zeroization, HTTPS `443`, NUT `3493`, refused `8080`, and full NUT recovery
  are covered before release.
