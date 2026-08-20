# v2.7.6 release evidence

## Publication

- Release: `v2.7.6`
- Merged source commit: `6616a5311` (pull request #44)
- Release tag: `v2.7.6`
- Target: YD-ESP32-23 / ESP32-S3-WROOM-1-N16R8, ESP-IDF v6.0.2, `esp32s3`
- Application artifact: `build/nut-esp32s3.bin`
- Artifact size: `1,339,296` bytes
- SHA-256: `9a4b06952ff29ae00c1fea3095c754eae423925f1d8d9ff94e0bac66e9dcd193`

## Implementation boundary

v2.7.6 adds a USB HID attachment-generation signal owned by `src/usb.c` and
driver-task-only replacement invalidation/reprobe in `usbhid-ups`. A changed
attachment clears cached external identity and measurements, resets report and
matcher state, and keeps NUT stale until the replacement completes a full
successful poll. USB callbacks do not call dstate or free driver resources.

The v2.7.7, v2.7.8, and v2.7.9 specification packages were merged with this
slice and remain active forward-plan documents.

## Validation

- `git diff --check`: passed.
- ESP-IDF v6.0.2 `esp32s3` reconfigure/build: passed.
- Embedded application version from the tagged build: `v2.7.6`.
- Authenticated OTA installation: HTTP 200, image verified, target restarted.
- Post-OTA target: firmware `v2.7.6`, update result `installed`, valid OTA slot.
- Post-OTA full poll: read-only NUT available, data current, health `ok`, and
  the connected CyberPower identity/measurements populated.
- Diagnostic simulation and bearer-scope isolation from the candidate: passed.

## Physical replacement evidence

- CyberPower-to-APC: first post-attachment responses remained stale with all
  external UPS fields unavailable; after reprobe and a full successful poll,
  only APC identity and measurements were exposed.
- APC-to-CyberPower: first post-attachment responses remained stale with all
  external UPS fields unavailable; after reprobe and a full successful poll,
  only CyberPower identity and measurements were exposed.
- No restart, manual driver restart, artificial reconnect wait, UPS control,
  or NVS change was used for the replacement tests.

## Boundaries and gaps

HTTPS `443`, read-only NUT `3493`, refused `8080`, ADMIN/CSRF, bearer-scope
isolation, and OTA rollback validity remain unchanged. Unsupported or malformed
replacement hardware was not target-tested. A rollback to v2.7.5 was not
needed or performed; the existing dual-OTA path remains the rollback plan.
