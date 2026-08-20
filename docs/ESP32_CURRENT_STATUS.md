# ESP32-NUT current development status

Read this after [AGENTS.md](../AGENTS.md), confirm it with live Git, and then
load only the document required for the active task. This is the single active
handoff for every agent, including context-limited agents. Do not preload the
archive, source tree, or project chat. Completed release evidence is in
[archive/](archive/README.md), not this startup handoff.

## Snapshot

| Field | Current fact |
| --- | --- |
| Canonical branch | `main` / `origin/main` |
| Published release | `v2.7.2` at `abf517f16`; resolve live Git before acting |
| Active preparation | `feature/nut-stale-timeout`, based from published `v2.7.2`; v2.7.3 implementation and target acceptance are complete locally but uncommitted |
| Validation | ESP-IDF v6.0.2 `esp32s3` build and `git diff --check` pass. The scoped Agent candidate captured one-time five-minute purge of 37 external values, stale protection, and expiry/full-poll recovery. A physical disconnect held beyond five minutes showed immediate stale invalidation and NUT `DATA-STALE`; uptime remained continuous and reconnect recovered only after a full poll. The bounded physical log snapshot rotated before it could retain the one-time message. |
| Target | YD-ESP32-23 / ESP32-S3-WROOM-1-N16R8, ESP-IDF v6.0.2, `esp32s3` |
| Required boundaries | LAN-only HTTPS `443`; read-only NUT `3493`; retired `8080` refused; ADMIN/CSRF and bearer-scope rules preserved |
| Management architecture | `management.c` is the root-policy, HTTPS-lifecycle, and factory-reset orchestration boundary; focused modules own the remaining management concerns |
| Last observed target result | The corrected v2.7.3 OTA candidate completed physical disconnect/reconnect and simulated expiry/recovery checks, then reported healthy read-only NUT data. Browser dashboard presentation was not rechecked on this candidate. |

## Current objective

v2.7.2 is complete: USB/HID loss, driver staleness, and bounded diagnostic
simulation use one management-data invalidation boundary. v2.7.3 adds a
monotonic five-minute stale-data purge, owned only by the normal driver poll
path. Next: review the uncommitted slice, commit it, and request publication
authority; do not publish until explicitly authorized.

## Read only when needed

| Need | Document |
| --- | --- |
| Active releases and branch scope | [ESP32_DEVELOPMENT_PLAN.md](ESP32_DEVELOPMENT_PLAN.md) |
| Active v2.7.3 implementation and acceptance contract | [ESP32_V2_7_3_STALE_TIMEOUT_SPEC.md](ESP32_V2_7_3_STALE_TIMEOUT_SPEC.md) |
| Released v2.7.2 acceptance evidence | [archive/v2.7.2/evidence.md](archive/v2.7.2/evidence.md) |
| Hardware, LAN, COM, build, flash, or OTA | [ESP32_PREFLIGHT.md](ESP32_PREFLIGHT.md) |
| Authority for physical, destructive, or external actions | [ESP32_DEVELOPMENT_ROLES.md](ESP32_DEVELOPMENT_ROLES.md) |
| Security and authorization boundaries | [ESP32_SECURITY.md](ESP32_SECURITY.md) |
| Completed management/Wi-Fi refactoring architecture | [ESP32_REFACTORING_PLAN.md](ESP32_REFACTORING_PLAN.md) |
| Completed v2.7.1 refactoring and release evidence | [archive/README.md](archive/README.md) |

Never record credentials, cookies, API tokens, private keys, or Authorization
headers here.
