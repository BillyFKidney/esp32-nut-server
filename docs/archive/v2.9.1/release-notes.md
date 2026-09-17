## Benefit

Wi-Fi scans now keep their short-lived driver result array in PSRAM, preserving
internal SRAM for timing-sensitive Wi-Fi and firmware work.

## What changed

The authenticated ADMIN and fallback portal scan paths share a PSRAM-first
allocation for at most 20 `wifi_ap_record_t` values (1,840 bytes on the target).
They retain an internal allocation fallback when PSRAM cannot satisfy the
request. Scan limits, ordering, deduplication, cleanup, provisioning behavior,
HTTPS `443`, read-only NUT `3493`, and refused `8080` are preserved.

## Verification

- Exact-tag ESP-IDF v6.0.2 build and size check with 59% app-slot headroom.
- Certificate-pinned OTA on 3Dprinter and authenticated 20-network-capped scan.
- Post-reboot healthy NUT/UPS `OL`, 57-variable read-only NUT poll, HTTPS `443`,
  NUT `3493`, and refused `8080`.

## Artifact

`nut-esp32s3-v2.9.1.bin` — SHA-256
`3d0f0478bb6ff9445cd42341d62f6285fd1609f0bcf227223bb3c1598935268d`
