# ESP32-NUT active development plan

This is the compact forward roadmap. Completed v2.7.1 scope, historical
sequencing, and released refactoring details are preserved in
[archive/ESP32_DEVELOPMENT_PLAN_V2_7_1.md](archive/ESP32_DEVELOPMENT_PLAN_V2_7_1.md).

## Version and publication rule

Each release is one independently reviewable, validated slice. A merge does
not publish a version: the Project Maintainer separately authorizes the tag,
release assets, and any target installation. Do not consume a later version
for completed maintenance work without updating this table first.

## Published baseline

`v2.7.1` is published and target-tested. It completed the management route
families, credential-promotion safety repair, and browser-log handshake filter
without changing the protected service, authorization, Wi-Fi, or read-only UPS
boundaries.

## Remaining 2.7.x sequence

| Release | Prospective branch | Required outcome |
| --- | --- | --- |
| `v2.7.2` | `feature/nut-disconnect-invalidation` | On UPS loss, status JSON and dashboard promptly mark data unavailable; stale values are never presented as current. |
| `v2.7.3` | `feature/nut-stale-timeout` | After five minutes without a confirmed connection, clear all UPS information while preserving an explicit stale/unavailable state. |
| `v2.7.4` | `feature/apc-br1500g-support` | Validate APC Back-UPS RS 1500G communication without freeze/reboot and report only confirmed identity/status/measurements. |
| `v2.7.5` | `feature/nut-compatibility-hardening` | Add bounded, graceful handling for supported and unsupported NUT-compatible UPS devices with a documented validation matrix. |
| `v2.7.6` | `feature/ups-change-without-wait` | Reprobe a replacement UPS immediately; prior identity and values cannot leak into the new device state. |
| `v2.7.7` | `feature/factory-reset-clears-state` | A 15-second-plus factory reset clears all defined saved user values, including UPS identity/cache state, while preserving firmware and recovery boundaries. |

## Guardrails

- Keep the inherited NUT daemon/driver architecture and read-only UPS access.
- Preserve LAN-only HTTPS `443`, read-only NUT `3493`, refused `8080`, and
  ADMIN/CSRF/bearer-token boundaries.
- Use ESP-IDF v6.0.2 on the ESP32-S3 target and validate each slice in
  proportion to its risk.
- Do not flash, OTA-install, reset, push, merge, tag, or release without
  explicit authority.
