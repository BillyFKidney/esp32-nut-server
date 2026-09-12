# ESP32-NUT v2.8.16 release evidence

- Source release commit: `0d613ca5d`
- Annotated tag: `v2.8.16`
- Firmware asset: `nut-esp32s3-v2.8.16.bin` (1,359,301 bytes)
- SHA-256: `dbc6d441918bb0b2579012b8fa4f06db9a251ea3e11d07110a4482405b36e065`

## Benefit

Diagnostic-duration maintenance is safer because the conversion unit is named
at its use site, reducing the risk of a future seconds-versus-microseconds
mistake without changing the diagnostic simulation users rely on.

## Validation

- Independent scan and review accepted the narrow constant extraction and
  preserved duration bounds, RAM-only state, diagnostic routes, and payloads.
- An isolated clean ESP-IDF v6.0.2 publication build from the exact tag and
  target configuration embedded `v2.8.16`, produced the recorded public
  artifact and SHA-256, and left 59% application-slot headroom. ESP-IDF binary
  output is build-context sensitive, so this public artifact differs bytewise
  from the separately built image used for live OTA acceptance.
- Scoped certificate-pinned OTA installed successfully on 3Dprinter. Pinned
  diagnostics report `v2.8.16`, normal Wi-Fi connection, update `installed`,
  and healthy NUT service.
