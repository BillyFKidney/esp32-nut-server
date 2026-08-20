# v2.7.5 release evidence

v2.7.5 hardens the read-only USB HID/NUT path for the evidenced CyberPower
CST150UC2 and APC Back-UPS RS 1500G devices. It adds bounded descriptor and
report handling, failed-claim cleanup, safe metadata termination, and wider
report-offset accounting without changing the management or NUT service
boundaries. Immediate replacement-UPS reprobe remains v2.7.6 scope.

## Publication

- Pull request: [#43](https://github.com/BillyFKidney/esp32-nut-server/pull/43)
- Merge commit: `396831d96769088c05504e1ee1fd7bd8a1809a21`
- Release tag: `v2.7.5`
- GitHub release: [ESP32-NUT v2.7.5](https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.7.5)
- Firmware asset: `nut-esp32s3.bin`
- Firmware SHA-256: `f591a2f09de19c49d7459e470c4c15ea58429ebdb5474bf46f589985004c178f`
- Firmware size: `1338832` bytes

## Automated validation

- `git diff --check` passed.
- ESP-IDF v6.0.2 `esp32s3` build passed.
- The tagged build reports application version `v2.7.5`.
- Exact tagged image was accepted and installed through the scoped OTA route.
- NUT remains read-only on TCP `3493`; retired port `8080` was closed.
- Certificate-pinned diagnostic status access succeeded after OTA.

## Target evidence

- CyberPower CST150UC2: healthy full-poll evidence was obtained on the
  commit-matched candidate before publication, including current identity and
  measurements.
- APC Back-UPS RS 1500G: the tagged v2.7.5 image reached `available=true`,
  `data_stale=false`, `health=ok`, and populated APC identity and measurements
  after the normal USB enumeration/poll retry interval.
- A first post-reboot sample may remain stale while USB/NUT starts; later
  successful full-poll data is required before declaring the device current.
- Unsupported or malformed HID hardware was not target-tested.
- Hot replacement without reboot was not part of this release; it remains the
  v2.7.6 acceptance boundary.

## Rollback

The prior target-accepted v2.7.4 release remains the rollback image. No NVS
migration, factory reset, token recreation, or UPS-control action was used.
Rollback execution was not required for this release.
