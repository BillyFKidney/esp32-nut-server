# ESP32-NUT v2.8.6 release evidence

## Release identity

- Source merge: `22e222bedebc1347ccfefe8849130479b52bc19a`
- Annotated tag: `v2.8.6`
- GitHub release: <https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.8.6>
- Firmware asset: `nut-esp32s3-v2.8.6.bin`
- Firmware size: 1,359,776 bytes
- SHA-256: `241f8aa1bf4e8a59e8723f896932224feedb6bc6ff0e8bc62c451f2fd4a46b7c`

## Scope

The dedicated review split private time-configuration persistence and timezone
policy from SNTP lifecycle and public time operations. It preserves the
`management` / `time-cfg` NVS record schema, version, defaults, supported IANA
timezone mappings, SNTP server lifetime, synchronization sequencing, manual
time rules, and ADMIN/CSRF route contracts.

## Validation

- Independent scanner and reviewer accepted the single `O-23` extraction in
  `.code-review-tracker.md`; no regression was found.
- The exact `v2.8.6` tag passed a clean ESP-IDF v6.0.2 reconfigure/build and
  size check with 59% application-slot headroom.
- Both 1Password environments passed redacted presence and structural-format
  validation before device work; only the authorized Agent Tests device was
  modified.
- The checksum-verified artifact OTA-installed successfully. Post-reboot,
  pinned HTTPS diagnostics reported `v2.8.6`, `installed`, and populated time
  configuration.
- HTTPS `443` and read-only NUT `3493` were available; retired `8080` was
  refused. A full CRLF-framed NUT poll returned 57 variables and
  `cyberpower ups.status` `OL`.

## Not tested

- Garage was not contacted for a mutation.
- The host lacked the `upsc` client, so the read-only full NUT acceptance used
  the documented protocol directly with CRLF framing.
