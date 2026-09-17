# ESP32-NUT v2.9.2 release evidence

## Scope

The authenticated browser OTA flow now holds a complete candidate only in
PSRAM until the ADMIN user explicitly selects **Install Firmware**. The check
route validates the ESP32 application image in PSRAM and returns an opaque,
single-use stage identifier. Install accepts an empty body plus that identifier,
copies only the checked image to the inactive OTA slot, lets ESP-IDF perform
its final validation, selects the slot, and restarts. The stage is replaced
atomically, expires after ten minutes, and is zeroized before release.

The browser ADMIN/CSRF boundary remains unchanged. The scoped Agent OTA route
continues to install directly and cannot access a browser stage. The detailed
contract and its secure-boot/anti-rollback configuration boundary are in
[the staged OTA contract](../../esp32/ESP32_V2_9_2_STAGED_OTA_CONTRACT.md).

## Candidate acceptance

- Clean candidate `f4e0d1325` built with ESP-IDF v6.0.2 and was installed on
  the certificate-pinned 3Dprinter through the scoped Agent route.
- An authenticated browser check staged the complete image in PSRAM while the
  device remained in `app1` with `app0` selected as next boot. The observed
  PSRAM-free decrease was consistent with retaining the complete image; no
  reboot occurred.
- A one-byte malformed browser upload returned HTTP 422. The previously
  checked stage remained available, and running/next slots stayed `app1` and
  `app0`; this proves rejection does not replace a good stage or write either
  slot.
- The authenticated check followed by explicit install returned `installed`,
  rebooted to the inactive slot, and released the staged PSRAM allocation.

## Exact-tag acceptance

- Annotated tag `v2.9.2` resolves to `f4e0d1325`.
- The exact ESP-IDF v6.0.2 build embedded `v2.9.2`, produced a 1,362,384-byte
  image with 59% app-slot headroom, and SHA-256
  `6845cbf094fed80eef29a28b40a6446c915bb77d96dea416fe5e9bff20e66abf`.
- The exact image first OTA-installed through the scoped Agent route, then
  passed the complete authenticated browser check-and-explicit-install route.
  Post-reboot diagnostics report `v2.9.2` in `app1`, `app0` as rollback slot,
  update `installed`, and 8 MiB PSRAM available.
- NUT recovered to `ok` with UPS `OL`; HTTPS `443` and read-only NUT `3493`
  responded, `8080` was refused, and read-only `LIST VAR` returned 57
  variables.
