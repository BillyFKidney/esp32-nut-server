# v2.7.7 release evidence

## Publication

- Release: `v2.7.7`
- Merged source commit: `2b94e56fa` (pull request #47)
- Release tag: `v2.7.7`
- GitHub release: `https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.7.7`
- Target: YD-ESP32-23 / ESP32-S3-WROOM-1-N16R8, ESP-IDF v6.0.2, `esp32s3`
- Application artifact: `nut-esp32s3-v2.7.7.bin`
- Artifact size: `1,339,392` bytes
- SHA-256: `0f1f1268902052747c663e36ec486e84a5df89cad8428ec0b8c6780ff8aed2ed`

## Implementation boundary

v2.7.7 defers BOOT recovery until release. A three-second hold erases only the
`wifi-config` namespace. A fifteen-second hold erases `management` first and
then `wifi-config`; it clears the RAM session and restarts only after both
operations succeed. Firmware, OTA metadata, and recovery partitions remain
untouched.

## Validation

- `git diff --check`: passed.
- Clean ESP-IDF v6.0.2 `esp32s3` build from tagged source: passed.
- Embedded application version from tagged build: `v2.7.7`.
- Authenticated tagged OTA installation and reboot: passed.
- Post-OTA target: tagged version, valid alternate OTA slot, update result
  `installed`, current read-only NUT data, and health `ok`.
- Three-second physical hold: Wi-Fi entered provisioning; ADMIN, time, and
  API-token configuration remained usable after reprovisioning.
- Fifteen-second physical hold: Wi-Fi and management state were erased; fresh
  ADMIN setup was required; prior diagnostic and OTA tokens were rejected.
- Fresh setup: replacement scoped tokens restored diagnostics and OTA access;
  time and OTA-result configuration began from reset state.
- Temporary management-erase fault injection: the target remained online,
  preserved Wi-Fi and ADMIN access, did not restart, and recorded the bounded
  failure. The injected image was replaced before publication.

## Boundaries and gaps

HTTPS `443`, read-only NUT `3493`, refused `8080`, ADMIN/CSRF, bearer-scope
isolation, and the UPS read-only policy remain unchanged. No rollback was
needed; the dual-OTA path remains available. Unsupported or malformed UPS
hardware was not retested by this release.
