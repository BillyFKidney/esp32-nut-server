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
| Active implementation | None; v2.7.2 is merged and published from `main` |
| Validation | `git diff --check`, Python probe syntax, ESP-IDF v6.0.2 `esp32s3` tagged build, pinned Agent authorization/simulation checks, and physical UPS disconnect/reconnect acceptance passed; the checksum-verified tagged image was OTA-installed and reported valid |
| Target | YD-ESP32-23 / ESP32-S3-WROOM-1-N16R8, ESP-IDF v6.0.2, `esp32s3` |
| Required boundaries | LAN-only HTTPS `443`; read-only NUT `3493`; retired `8080` refused; ADMIN/CSRF and bearer-scope rules preserved |
| Management architecture | `management.c` is the root-policy, HTTPS-lifecycle, and factory-reset orchestration boundary; focused modules own the remaining management concerns |
| Last observed target result | v2.7.2 was OTA-installed, marked valid, and reported healthy read-only NUT data; detailed evidence is archived |

## Current objective

Begin from live `main`, not a historical feature branch. v2.7.2 is complete:
USB/HID loss, driver staleness, and bounded diagnostic simulation use one
management-data invalidation boundary. Next: define and implement the separate
v2.7.3 stale-timeout slice only after its scope and authorization are current.

## Read only when needed

| Need | Document |
| --- | --- |
| Active releases and branch scope | [ESP32_DEVELOPMENT_PLAN.md](ESP32_DEVELOPMENT_PLAN.md) |
| v2.7.2 implementation and acceptance package | [ESP32_V2_7_2_DISCONNECT_INVALIDATION_SPEC.md](ESP32_V2_7_2_DISCONNECT_INVALIDATION_SPEC.md) |
| Hardware, LAN, COM, build, flash, or OTA | [ESP32_PREFLIGHT.md](ESP32_PREFLIGHT.md) |
| Authority for physical, destructive, or external actions | [ESP32_DEVELOPMENT_ROLES.md](ESP32_DEVELOPMENT_ROLES.md) |
| Security and authorization boundaries | [ESP32_SECURITY.md](ESP32_SECURITY.md) |
| Completed management/Wi-Fi refactoring architecture | [ESP32_REFACTORING_PLAN.md](ESP32_REFACTORING_PLAN.md) |
| Completed v2.7.1 refactoring and release evidence | [archive/README.md](archive/README.md) |

Never record credentials, cookies, API tokens, private keys, or Authorization
headers here.
