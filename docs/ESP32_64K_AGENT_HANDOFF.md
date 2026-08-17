# ESP32-NUT 64k-agent handoff

This compact continuation packet leaves room for the active source slice. Do
not preload the archive, full source tree, or project conversation.

## Read order

1. [AGENTS.md](../AGENTS.md)
2. [Current status](ESP32_CURRENT_STATUS.md)
3. This handoff
4. [Route inventory](ESP32_ROUTE_INVENTORY.md) and
   [refactoring plan](ESP32_REFACTORING_PLAN.md)
5. Only the source and headers required by the active route family

Read `ESP32_PREFLIGHT.md` only for explicitly authorized device work.

## Canonical continuation state

- Canonical code base: `main`; live Git is authoritative.
- Active branch: `feature/management-route-families`, based on `main` at
  `91079a7f5`; confirm branch, HEAD, and worktree before acting.
- Target: YD-ESP32-23 / ESP32-S3-WROOM-1-N16R8, ESP-IDF v6.0.2, `esp32s3`.
- Preserve: HTTPS `443`, read-only NUT `3493`, refused `8080`, ADMIN/CSRF,
  bearer scopes, Authorization-header zeroization, Wi-Fi recovery, and
  read-only UPS access.
- The v2.7.0-40 candidate passed management, Wi-Fi, token, time, and
  read-only UPS checks. Its password-change safety repair also passed two
  password rotations and a subsequent new-password sign-in. The next
  diagnostic-only slice filters the routine ESP-IDF HTTPS handshake message
  from the browser log snapshot; serial output and TLS errors remain intact.
  Do not alter the device without fresh authority.

## Current architecture

`management.c` has delegated pages, authentication routes, session routes,
status/time/token/Wi-Fi/OTA route families, credentials, sessions,
certificates, HTTP helpers, status snapshots, logs, shared authorization, and
route composition to focused modules. It contains only root-page policy, HTTPS
bootstrap, and factory reset.

The final refactor preserves `management.c` as an orchestration boundary. It
will retain only:

- root-page setup/login/ADMIN policy and CSRF copying/zeroization;
- HTTPS server lifecycle; and
- `management_factory_reset()` until the separate v2.7.1 factory-reset slice.

## Completed route-family sequence

1. `management-status-routes.c` — status route only, because its session check
   must not record ADMIN activity.
2. `management-time-routes.c` — time configuration.
3. `management-token-routes.c` — token list/create/delete together.
4. `management-wifi-routes.c` — scan/configuration together.
5. `management-ota-routes.c` — browser check/install and bearer Agent install
   together.
6. `management-routes.c` — all 17 descriptor definitions and exact registration
   order, after their handlers are external.

This groups cohesive API families, not individual routes. Each source is
explicitly registered in `src/CMakeLists.txt` and has a narrow public header.

## Validation contract

ESP-IDF v6.0.2 builds passed after each family, final composition, and the
password-safety repair. The complete route-family diff, handler-body transfers,
and route order were reviewed; `git diff --check` passed. Build the log-noise
filter, then stop at a clean candidate for explicit upload and one browser-log
snapshot test. Do not flash, OTA, reset, push, merge, tag, or release without
fresh authority.
