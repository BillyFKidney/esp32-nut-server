# ESP32-NUT v2.8.7 release evidence

## Release identity

- Source merge: `5b0e058e96bef77ed4db02cb395c21c7c03799a8`
- Annotated tag: `v2.8.7`
- GitHub release: <https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.8.7>
- Firmware asset: `nut-esp32s3-v2.8.7.bin`
- Firmware size: 1,359,680 bytes
- SHA-256: `ccdea986330ea48aba3e8df7cbdf51374dceeda63e600664ffccf137c77fee9a`

## Scope

The one-item review centralizes the runtime-log single-character level mapping
used by the six-entry status window and 24-entry retained-log route. The
mapping remains `E` to `error`, `W` to `warning`, `D` and `V` to `debug`, and
all other values to `info`.

## Validation

- Independent scanner found one real, bounded `CC-1` duplicated-mapping issue;
  the independent reviewer accepted the surgical extraction with no regression.
- The exact `v2.8.7` tag passed a clean ESP-IDF v6.0.2 reconfigure/build and
  size check with 59% application-slot headroom.
- Both 1Password environments passed redacted presence and structural-format
  validation before device work; only the authorized Agent Tests device was
  modified.
- The checksum-verified artifact OTA-installed successfully. Post-reboot,
  pinned diagnostics reported `v2.8.7`, `installed`, and a six-entry-or-less
  status-log window.
- Authenticated ADMIN validation passed the rendered page, status window,
  24-entry full-log contract, unauthenticated full-log rejection, and invalid
  CSRF rejection.
- HTTPS `443` and read-only NUT `3493` were available; retired `8080` was
  refused. A full CRLF-framed NUT poll returned 57 variables and
  `cyberpower ups.status` `OL`.

## Not tested

- Garage was not contacted for a mutation.
- The host lacks the `upsc` client, so full NUT acceptance used the protocol
  directly with CRLF framing.
