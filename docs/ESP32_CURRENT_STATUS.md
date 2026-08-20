# ESP32-NUT current development status

Read this after [AGENTS.md](../AGENTS.md), confirm it with live Git, and then
load only the document required for the active task. This is the single active
handoff for every agent, including context-limited agents. Do not preload the
archive, source tree, or project chat. Completed release evidence is in
[archive/](archive/README.md), not this startup handoff.

## Snapshot

| Field | Current fact |
| --- | --- |
| Canonical branch | `feature/apc-br1500g-support` from `main` at `55d0aa4dd16b` |
| Published release | `v2.7.3`; v2.7.4 publication is authorized and in progress |
| Active preparation | v2.7.4 APC Back-UPS RS 1500G compatibility fix is implemented and target-accepted on this branch |
| Validation | ESP-IDF v6.0.2 `esp32s3` build and `git diff --check` pass. The candidate was OTA-installed, enumerated the APC, completed full polls, and remained healthy for more than 17 minutes of continuous authenticated status observation. Three APC disconnect/reconnect cycles, a CyberPower boot/disconnect/stale-invalidation/full-poll recovery cycle, browser dashboard presentation, service ports, and token isolation passed. One earlier observation had an unexpected reboot near 13 minutes and did not reproduce during the extended run; the release decision explicitly accepts this observed but unreproduced event. |
| Target | YD-ESP32-23 / ESP32-S3-WROOM-1-N16R8, ESP-IDF v6.0.2, `esp32s3` |
| Required boundaries | LAN-only HTTPS `443`; read-only NUT `3493`; retired `8080` refused; ADMIN/CSRF and bearer-scope rules preserved |
| Management architecture | `management.c` is the root-policy, HTTPS-lifecycle, and factory-reset orchestration boundary; focused modules own the remaining management concerns |
| Last observed target result | The v2.7.4 candidate reports CyberPower identity, `OL`, healthy read-only NUT data, and dashboard values after the final reboot/recovery check. |

## Current objective

v2.7.2 is complete: USB/HID loss, driver staleness, and bounded diagnostic
simulation use one management-data invalidation boundary. v2.7.3 adds a
monotonic five-minute stale-data purge, owned only by the normal driver poll
path. v2.7.4 now removes the restrictive CyberPower-only HID filters while
preserving the established NUT service name, allowing the evidenced APC HID
subdriver to claim the device. Next: review the candidate diff and decide
whether to authorize commit/publication; CyberPower regression and browser
dashboard presentation passed; the earlier unreproduced reboot is retained as
historical evidence, with publication explicitly authorized by the maintainer.

## Read only when needed

| Need | Document |
| --- | --- |
| Active releases and branch scope | [ESP32_DEVELOPMENT_PLAN.md](ESP32_DEVELOPMENT_PLAN.md) |
| Active v2.7.3 implementation and acceptance contract | [ESP32_V2_7_3_STALE_TIMEOUT_SPEC.md](ESP32_V2_7_3_STALE_TIMEOUT_SPEC.md) |
| Released v2.7.4 APC compatibility evidence | [archive/v2.7.4/evidence.md](archive/v2.7.4/evidence.md) |
| Next v2.7.5 compatibility hardening | [ESP32_V2_7_5_NUT_COMPATIBILITY_SPEC.md](ESP32_V2_7_5_NUT_COMPATIBILITY_SPEC.md) |
| Released v2.7.2 acceptance evidence | [archive/v2.7.2/evidence.md](archive/v2.7.2/evidence.md) |
| Hardware, LAN, COM, build, flash, or OTA | [ESP32_PREFLIGHT.md](ESP32_PREFLIGHT.md) |
| Authority for physical, destructive, or external actions | [ESP32_DEVELOPMENT_ROLES.md](ESP32_DEVELOPMENT_ROLES.md) |
| Security and authorization boundaries | [ESP32_SECURITY.md](ESP32_SECURITY.md) |
| Completed management/Wi-Fi refactoring architecture | [ESP32_REFACTORING_PLAN.md](ESP32_REFACTORING_PLAN.md) |
| Completed v2.7.1 refactoring and release evidence | [archive/README.md](archive/README.md) |

Never record credentials, cookies, API tokens, private keys, or Authorization
headers here.
