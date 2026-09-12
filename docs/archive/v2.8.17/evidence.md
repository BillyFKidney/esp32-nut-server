# ESP32-NUT v2.8.17 release evidence

- Source release commit: `9e476eb97`
- Annotated tag: `v2.8.17`
- Firmware asset: `nut-esp32s3-v2.8.17.bin` (1,359,301 bytes)
- SHA-256: `460dac2f6f3f1af5804de79ee458319057954f25ae6fa7205507309785cc07da`

## Benefit

Private time-storage maintenance is easier to audit because its standard
headers follow the project’s import grouping, without changing saved time
settings, timezone behavior, or runtime behavior.

## Validation

- Independent scan and review accepted the one-line import-order change and
  preserved NVS persistence, timezone behavior, and runtime contracts.
- A clean ESP-IDF v6.0.2 build from the exact tag embedded `v2.8.17`, produced
  the recorded artifact and SHA-256, and left 59% application-slot headroom.
- Scoped certificate-pinned OTA installed successfully on 3Dprinter. Pinned
  diagnostics reported `v2.8.17`, normal Wi-Fi, update `installed`, and a
  healthy NUT service.
- HTTPS `443` and read-only NUT `3493` responded, `8080` was refused, and the
  complete raw NUT poll reported UPS status `OL`.
