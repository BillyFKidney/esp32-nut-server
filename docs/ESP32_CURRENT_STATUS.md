# ESP32-NUT current development status

Read this after [AGENTS.md](../AGENTS.md), confirm it with live Git, and then
load only the document required for the active task. This is the single active
handoff for every agent, including context-limited agents. Do not preload the
archive, source tree, or project chat. Completed release evidence is in
[archive/](archive/README.md), not this startup handoff.

## Snapshot

| Field | Current fact |
| --- | --- |
| Canonical branch | `main` at v2.7.7 merge commit `2b94e56fa` |
| Published release | `v2.7.7` tag points to source merge commit `2b94e56fa`; release evidence is in [archive/v2.7.7/evidence.md](archive/v2.7.7/evidence.md) |
| Active implementation | v2.7.8 status UI/API naming is the next isolated release slice |
| Validation | v2.7.7 `git diff --check`, clean ESP-IDF v6.0.2 `esp32s3` build, tagged OTA install, physical reset acceptance, scoped-token recovery, and management-NVS failure injection pass. |
| Target | YD-ESP32-23 / ESP32-S3-WROOM-1-N16R8, ESP-IDF v6.0.2, `esp32s3` |
| Required boundaries | LAN-only HTTPS `443`; read-only NUT `3493`; retired `8080` refused; ADMIN/CSRF and bearer-scope rules preserved |
| Management architecture | `management.c` is the root-policy, HTTPS-lifecycle, and factory-reset orchestration boundary; focused modules own the remaining management concerns |
| Last observed target result | Tagged `v2.7.7` is OTA-installed and, after reboot, reports current read-only NUT data following a full successful poll. |

## Current objective

v2.7.2 is complete: USB/HID loss, driver staleness, and bounded diagnostic
simulation use one management-data invalidation boundary. v2.7.3 adds a
monotonic five-minute stale-data purge, owned only by the normal driver poll
path. v2.7.4 removed the restrictive CyberPower-only HID filters while
preserving the established NUT service name, allowing the evidenced APC HID
subdriver to claim the device. The release is published and installed; the
earlier unreproduced reboot is retained as historical evidence. v2.7.5 now
begins from this published, target-installed v2.7.4 baseline. The candidate
hardens descriptor bounds, allocation cleanup, metadata termination, and
unsupported/malformed HID handling without changing the read-only service or
reconnect cadence. A valid report descriptor can exceed 255 bits before its
later fields; the candidate now uses 16-bit report offsets while retaining
overflow bounds. v2.7.5 is published and target-accepted; unsupported or
malformed hardware remains explicitly not target-tested. v2.7.6 now adds a
USB attachment generation and driver-task-only broad reprobe: replacement
identity is cleared before probing, old HID mappings and exact matchers are
discarded, and only a full successful poll can make the replacement current.
The candidate is built with ESP-IDF v6.0.2 and installed through authorized
OTA. Automated simulation and token-isolation checks pass. Both physical
directions were observed: each first returned stale/unavailable data, then
exposed only the replacement after a successful reprobe/full poll. No
unsupported or malformed replacement hardware is target-tested. The clean
tagged artifact is published and OTA-installed. v2.7.7 defers reset operations
until BOOT release; its fifteen-second path erases management then Wi-Fi, clears
the RAM session only after both succeed, and restarts only after complete
success. Physical Wi-Fi-only and factory-reset recovery, fresh setup,
credential invalidation/recovery, and an injected management-erase failure all
passed. The published evidence records the exact artifact and validation
boundary. Next exact action: apply the isolated v2.7.8 status UI/API slice to a
new release branch.

## Read only when needed

| Need | Document |
| --- | --- |
| Active releases and branch scope | [ESP32_DEVELOPMENT_PLAN.md](ESP32_DEVELOPMENT_PLAN.md) |
| Released v2.7.7 factory-reset evidence | [archive/v2.7.7/evidence.md](archive/v2.7.7/evidence.md) |
| v2.7.8 status UI acceptance contract | [ESP32_V2_7_8_STATUS_UI_SPEC.md](ESP32_V2_7_8_STATUS_UI_SPEC.md) |
| Active v2.7.3 implementation and acceptance contract | [ESP32_V2_7_3_STALE_TIMEOUT_SPEC.md](ESP32_V2_7_3_STALE_TIMEOUT_SPEC.md) |
| Released v2.7.4 APC compatibility evidence | [archive/v2.7.4/evidence.md](archive/v2.7.4/evidence.md) |
| Released v2.7.5 compatibility hardening | [archive/v2.7.5/evidence.md](archive/v2.7.5/evidence.md) |
| Released v2.7.6 replacement-UPS reprobe | [archive/v2.7.6/ESP32_V2_7_6_UPS_REPLACEMENT_SPEC.md](archive/v2.7.6/ESP32_V2_7_6_UPS_REPLACEMENT_SPEC.md) |
| Released v2.7.2 acceptance evidence | [archive/v2.7.2/evidence.md](archive/v2.7.2/evidence.md) |
| Hardware, LAN, COM, build, flash, or OTA | [ESP32_PREFLIGHT.md](ESP32_PREFLIGHT.md) |
| Authority for physical, destructive, or external actions | [ESP32_DEVELOPMENT_ROLES.md](ESP32_DEVELOPMENT_ROLES.md) |
| Security and authorization boundaries | [ESP32_SECURITY.md](ESP32_SECURITY.md) |
| Completed management/Wi-Fi refactoring architecture | [ESP32_REFACTORING_PLAN.md](ESP32_REFACTORING_PLAN.md) |
| Completed v2.7.1 refactoring and release evidence | [archive/README.md](archive/README.md) |

Never record credentials, cookies, API tokens, private keys, or Authorization
headers here.
