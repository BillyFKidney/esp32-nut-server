# v2.8.5 release evidence

## Scope

The v2.8.5 review release isolates Wi-Fi scan record collection and
deduplication, restores AP-list cleanup after a record-retrieval failure, and
removes an unreachable duplicate physical button-release loop. It preserves
provisioning, credential, synchronization, and route behavior.

## Release source and artifact

- Source: annotated tag `v2.8.5` at `9a4f86217405a2c4c9e7407139a1a470ef53c888`.
- Build: clean exact-tag ESP-IDF v6.0.2 reconfigure/build and size check.
- Artifact: `nut-esp32s3-v2.8.5.bin`, 1,359,760 bytes.
- SHA-256: `809ea0a7ac2570cf902ae42bafcca9449fd61a3e7286983150d4fb5e88a4deb5`.
- Publication: [GitHub release v2.8.5](https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.8.5), including the firmware and SHA-256 sidecar.

## Acceptance

- Both Agent Tests and Garage 1Password variable inventories passed redacted
  presence and format validation; Garage was not modified.
- Independent focused scan and source review passed against the locked Wi-Fi
  scan contract. ESP-IDF v6.0.2 documentation confirms successful
  `esp_wifi_scan_get_ap_records()` consumes the driver AP list.
- An authenticated Wi-Fi scan returned HTTP 200 with nine network results and
  the configured maximum of 20.
- The exact tagged artifact installed through the scoped Agent OTA route.
- After reboot, diagnostics reported firmware `v2.8.5` and update `installed`.
- HTTPS `443` and read-only NUT `3493` were reachable; retired `8080` was refused.
- A complete NUT `LIST VAR` poll returned 57 variables and `ups.status` `OL`.
