# v2.7.8 release evidence

## Publication

- Release: `v2.7.8`
- Merged source commit: `9fb925954` (pull request #49)
- Release tag: `v2.7.8`
- GitHub release: `https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.7.8`
- Target: YD-ESP32-23 / ESP32-S3-WROOM-1-N16R8, ESP-IDF v6.0.2, `esp32s3`
- Application artifact: `nut-esp32s3-v2.7.8.bin`
- Artifact size: `1,339,456` bytes
- SHA-256: `f5aca639fc503a6d1aeb3b509ab1b168964123a59490da64c0839b1b28f6990d`

## Implementation boundary

v2.7.8 renames the configured NUT service identity in browser and Agent status
from `nut.ups_name` to `nut.ups` without an alias. The dashboard Model uses
only current physical manufacturer/model data, and Device Status raw JSON is
expanded on initial rendering. It adds no UPS control, persistence, endpoint,
or authorization behavior.

## Validation

- `git diff --check`: passed.
- Clean ESP-IDF v6.0.2 `esp32s3` build from tagged source: passed.
- Embedded application version from tagged build: `v2.7.8`.
- Authenticated tagged OTA installation and reboot: passed.
- Browser and diagnostic-bearer status: contain `nut.ups`, omit
  `nut.ups_name`, and preserve the configured service identity.
- ADMIN page emitted Device Status with `<details open>`; the dashboard Model
  rendered the current physical manufacturer/model without the service name.
- Diagnostic disconnect simulation: browser and Agent status reported
  unavailable physical identity while stale and current identity only after
  recovery/full poll.
- ADMIN status reads did not refresh the session; diagnostic bearer remained
  unauthorized for OTA; HTTPS `443`, read-only NUT `3493`, and refused `8080`
  were preserved.
- A recovery serial flash of the exact tagged image preserved NVS-backed Wi-Fi,
  management, time, and token configuration. A clean-slot v2.7.8 OTA then
  installed successfully on `app1`.
- Rollback: tagged v2.7.7 installed on `app0`, returned the legacy
  `nut.ups_name` contract, and completed a healthy full poll. Tagged v2.7.8
  then restored on `app1`, returned only `nut.ups`, and completed a healthy
  full poll.

## Boundaries and gaps

HTTPS `443`, read-only NUT `3493`, refused `8080`, ADMIN/CSRF, bearer-scope
isolation, and the UPS read-only policy remain unchanged. The physical target
is restored to v2.7.8 on `app1`. Unsupported or malformed UPS hardware was
not retested by this release.
