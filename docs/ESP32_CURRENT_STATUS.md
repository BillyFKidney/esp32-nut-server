# ESP32-NUT current development status

Read this after [AGENTS.md](../AGENTS.md), confirm it with live Git, and then
load only the document required for the active task. Completed release evidence
is in [archive/](archive/README.md), not this startup handoff.

## Snapshot

| Field | Current fact |
| --- | --- |
| Canonical branch | `main` / `origin/main` |
| Published release | `v2.7.1` at `163e3d08d`; resolve live Git before acting |
| Worktree at update | Clean |
| Target | YD-ESP32-23 / ESP32-S3-WROOM-1-N16R8, ESP-IDF v6.0.2, `esp32s3` |
| Required boundaries | LAN-only HTTPS `443`; read-only NUT `3493`; retired `8080` refused; ADMIN/CSRF and bearer-scope rules preserved |
| Last observed target result | v2.7.1 was installed and all requested release tests passed; detailed evidence is archived |

## Current objective

Start `feature/nut-disconnect-invalidation` from current `main`. Reproduce the
disconnected-UPS stale-value case, define the invalidation acceptance boundary,
and change no device state without fresh authorization.

## Read only when needed

| Need | Document |
| --- | --- |
| Active releases and branch scope | [ESP32_DEVELOPMENT_PLAN.md](ESP32_DEVELOPMENT_PLAN.md) |
| Hardware, LAN, COM, build, flash, or OTA | [ESP32_PREFLIGHT.md](ESP32_PREFLIGHT.md) |
| Authority for physical, destructive, or external actions | [ESP32_DEVELOPMENT_ROLES.md](ESP32_DEVELOPMENT_ROLES.md) |
| Security and authorization boundaries | [ESP32_SECURITY.md](ESP32_SECURITY.md) |
| Completed v2.7.1 refactoring and release evidence | [archive/README.md](archive/README.md) |

Never record credentials, cookies, API tokens, private keys, or Authorization
headers here.
