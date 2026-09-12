# ESP32-NUT v2.8.15 release evidence

- Source release commit: `018f418b2`
- Annotated tag: `v2.8.15`
- GitHub release: <https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.8.15>
- Firmware asset: `nut-esp32s3-v2.8.15.bin` (1,359,301 bytes)
- SHA-256: `6e18aeb593bb71dd472a5828e2e52a452a5f6e416928b3164d120c02722758da`

## Benefit

Future Wi-Fi storage maintenance is less likely to let active and recovery
credential handling drift apart, while the device keeps its existing saved
network, provisioning, validation, and recovery behavior.

## Validation

- Independent scan and review accepted the private fixed-key helper while
  preserving NVS namespace and keys, read/write order, commit/close behavior,
  errors, and caller-owned credential buffers.
- A clean ESP-IDF v6.0.2 build from the exact tag embedded `v2.8.15`, produced
  the recorded artifact and SHA-256, and left 59% application-slot headroom.
- Scoped certificate-pinned OTA installed successfully on 3Dprinter. The
  device returned to its unchanged `ClubHouse_IoT` station configuration;
  diagnostics reported `v2.8.15`, `installed`, and healthy NUT data.
- HTTPS `443` and read-only NUT `3493` responded, `8080` was refused, and the
  complete raw NUT poll reported UPS status `OL`.
