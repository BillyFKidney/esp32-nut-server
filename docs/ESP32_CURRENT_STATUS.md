# ESP32-NUT current development status

Read this after [AGENTS.md](../AGENTS.md), confirm it with live Git, and then
load only the document required for the active task. This is the single active
handoff for every agent, including context-limited agents. Do not preload the
archive, source tree, or project chat. Completed release evidence is in
[archive/](archive/README.md), not this startup handoff.

## Snapshot

| Field | Current fact |
| --- | --- |
| Canonical branch | `feature/nut-compatibility-hardening` from `main` at `c728c750a` |
| Published release | `v2.7.4` on `main` at merge commit `8a52c8991` |
| Active preparation | v2.7.5 NUT compatibility hardening is authorized; parser/backend safety changes are implemented on this branch and the first ESP-IDF candidate build passes |
| Validation | `git diff --check` and ESP-IDF v6.0.2 `esp32s3` build pass. The authorized candidate was OTA-installed and restarted cleanly, but the post-OTA target remained NUT-stale with no HID device observed, so healthy CyberPower regression and physical compatibility acceptance remain pending. The published v2.7.4 APC/CyberPower, stale/recovery, dashboard, service-port, and token-isolation evidence remains the baseline. |
| Target | YD-ESP32-23 / ESP32-S3-WROOM-1-N16R8, ESP-IDF v6.0.2, `esp32s3` |
| Required boundaries | LAN-only HTTPS `443`; read-only NUT `3493`; retired `8080` refused; ADMIN/CSRF and bearer-scope rules preserved |
| Management architecture | `management.c` is the root-policy, HTTPS-lifecycle, and factory-reset orchestration boundary; focused modules own the remaining management concerns |
| Last observed target result | v2.7.5 candidate OTA restarted cleanly and reports `update=installed`, but currently shows NUT `available=false`, `data_stale=true`, and all external UPS fields unavailable; physical HID presence/healthy poll must be re-established before acceptance. |

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
reconnect cadence. Next: reconnect an evidenced UPS and capture healthy
Agent/NUT and physical stale/recovery evidence before acceptance.

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
