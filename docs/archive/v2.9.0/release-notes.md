## Benefit

Management status and retained-log requests now use PSRAM for their bounded
bulk response storage, preserving scarce internal SRAM for timing-sensitive
firmware work.

## What changed

The 7,000-byte status JSON response and full-log snapshot explicitly request
byte-addressable PSRAM. ADMIN/CSRF and bearer authorization, response payloads,
error handling, zeroization, HTTPS `443`, read-only NUT `3493`, and refused
`8080` are preserved.

## Verification

- Exact-tag ESP-IDF v6.0.2 build with 59% app-slot headroom.
- Certificate-pinned OTA on 3Dprinter, then authenticated status/full-log and
  unauthorized/invalid-CSRF acceptance.
- HTTPS `443`, read-only NUT `3493`, refused `8080`, and UPS `OL` after reboot.

## Artifact

`nut-esp32s3-v2.9.0.bin` — SHA-256
`a116275bbb0b24cacf1155a20e8444a7568f47f8789a7168c730e7858e375f0c`
