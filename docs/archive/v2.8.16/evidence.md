# ESP32-NUT v2.8.16 release evidence

- Source release commit: `0d613ca5d`
- Annotated tag: `v2.8.16`
- Firmware asset: `nut-esp32s3-v2.8.16.bin` (1,359,301 bytes)
- SHA-256: `7075bf4b05faef20e5a8eed74c5aa1ee2994046ab2c343b7ac31ae63701e16b0`

## Benefit

Diagnostic-duration maintenance is safer because the conversion unit is named
at its use site, reducing the risk of a future seconds-versus-microseconds
mistake without changing the diagnostic simulation users rely on.

## Validation

- Independent scan and review accepted the narrow constant extraction and
  preserved duration bounds, RAM-only state, diagnostic routes, and payloads.
- A clean ESP-IDF v6.0.2 build from the exact tag embedded `v2.8.16`, produced
  the recorded artifact and SHA-256, and left 59% application-slot headroom.
- Scoped certificate-pinned OTA installed successfully on 3Dprinter. Pinned
  diagnostics report `v2.8.16`, normal Wi-Fi connection, update `installed`,
  and healthy NUT service.
