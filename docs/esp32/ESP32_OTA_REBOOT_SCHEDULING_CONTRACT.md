# OTA reboot-scheduling contract

## Scope

This bounded `v2.8.Z` review addresses only the failure path in
`ota_process_from_request()` after an image has passed validation and its
partition has been selected for the next boot.

## Required behavior

- A successful install response may be emitted only when a reboot has been
  scheduled.
- If FreeRTOS cannot create the delayed reboot task after the success response
  has been queued, restart immediately so the selected, verified image is not
  left pending without a restart path.
- Preserve the existing success status/body, one-second delayed normal path,
  NVS result names, image validation, partition selection, authorization
  boundary, and check-only behavior.

## Verification

Require independent review, an exact ESP-IDF v6.0.2 build, and the established
scoped OTA, service-boundary, and post-reboot NUT acceptance before a release
version is assigned.
