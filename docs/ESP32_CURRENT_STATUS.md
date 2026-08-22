# ESP32-NUT current development status

Read this after [AGENTS.md](../AGENTS.md), confirm it with live Git, and then
load only the document required for the active task. This is the single active
handoff for every agent, including context-limited agents. Do not preload the
archive, source tree, or project chat. Completed release evidence is in
[archive/](archive/README.md), not this startup handoff.

## Snapshot

| Field | Current fact |
| --- | --- |
| Canonical branch | `main` at post-release documentation update based on tagged commit `bb92582b7` |
| Published release | [`v2.7.9`](https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.7.9) tag points to `bb92582b7`; release evidence is in [archive/v2.7.9/evidence.md](archive/v2.7.9/evidence.md) |
| Active implementation | v2.7.9 device identity, retained log level, and status-response stack-pressure repair are published and target-accepted |
| Validation | `git diff --check`, clean tagged ESP-IDF v6.0.2 `esp32s3` build, exact checksum verification, authenticated OTA install, ADMIN save of device name and log level, CSRF rejection on the new admin route, reboot persistence, browser/Agent status, stale/recovery, service-boundary, serial recovery, factory-reset clearing, panic reproduction, heap-buffer fix, token-scope/limit validation, certificate validation, read-only NUT validation, and post-OTA status-load validation pass. |
| Target | YD-ESP32-23 / ESP32-S3-WROOM-1-N16R8, ESP-IDF v6.0.2, `esp32s3` |
| Required boundaries | LAN-only HTTPS `443`; read-only NUT `3493`; retired `8080` refused; ADMIN/CSRF and bearer-scope rules preserved |
| Management architecture | `management.c` is the root-policy, HTTPS-lifecycle, and factory-reset orchestration boundary; focused modules own the remaining management concerns |
| Last observed target result | The target reports `v2.7.9` on `app0`, `update.last_result=installed`, `device_name=3dprinter-cyberpower`, `hostname=3dprinter-cyberpower`, `log_level=error`, healthy read-only NUT data, and 15 consecutive healthy status responses after OTA. |

## Current objective

Next exact action: begin the separately scoped v2.7.10 status-UI polish slice.

## Read only when needed

| Need | Document |
| --- | --- |
| Active releases and branch scope | [ESP32_DEVELOPMENT_PLAN.md](ESP32_DEVELOPMENT_PLAN.md) |
| Hardware, LAN, COM, build, flash, or OTA | [ESP32_PREFLIGHT.md](ESP32_PREFLIGHT.md) |
| Authority for physical, destructive, or external actions | [ESP32_DEVELOPMENT_ROLES.md](ESP32_DEVELOPMENT_ROLES.md) |
| Security and authorization boundaries | [ESP32_SECURITY.md](ESP32_SECURITY.md) |
| Completed management/Wi-Fi refactoring architecture | [ESP32_REFACTORING_PLAN.md](ESP32_REFACTORING_PLAN.md) |
| Released v2.7.8 status UI evidence | [archive/v2.7.8/evidence.md](archive/v2.7.8/evidence.md) |
| Released v2.7.7 factory-reset evidence | [archive/v2.7.7/evidence.md](archive/v2.7.7/evidence.md) |
| Released v2.7.6 replacement-UPS reprobe | [archive/v2.7.6/ESP32_V2_7_6_UPS_REPLACEMENT_SPEC.md](archive/v2.7.6/ESP32_V2_7_6_UPS_REPLACEMENT_SPEC.md) |
| Released v2.7.5 compatibility hardening | [archive/v2.7.5/evidence.md](archive/v2.7.5/evidence.md) |
| Released v2.7.4 APC compatibility evidence | [archive/v2.7.4/evidence.md](archive/v2.7.4/evidence.md) |
| Released v2.7.3 implementation and acceptance contract | [archive/v2.7.3/ESP32_V2_7_3_STALE_TIMEOUT_SPEC.md](archive/v2.7.3/ESP32_V2_7_3_STALE_TIMEOUT_SPEC.md) |
| Released v2.7.2 acceptance evidence | [archive/v2.7.2/evidence.md](archive/v2.7.2/evidence.md) |
| Released v2.7.1 refactoring and release evidence | [archive/README.md](archive/README.md) |

Never record credentials, cookies, API tokens, private keys, or Authorization
headers here.
