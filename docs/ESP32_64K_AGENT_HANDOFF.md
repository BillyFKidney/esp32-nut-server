# ESP32-NUT 64k-agent handoff

Read in order: [AGENTS.md](../AGENTS.md),
[current status](ESP32_CURRENT_STATUS.md), this file, then the active task
document. Do not preload `docs/archive/`, the source tree, or project chat.

## Starting point

- Begin from live `main`, not a historical feature branch.
- Next slice: `feature/nut-disconnect-invalidation` for `v2.7.2`.
- Preserve HTTPS `443`, read-only NUT `3493`, refused `8080`, ADMIN/CSRF,
  bearer scopes, Authorization-header zeroization, Wi-Fi recovery, and
  read-only UPS access.
- Treat device addresses, serial paths, installed firmware, and USB state as
  observations to rediscover, not configuration to copy from history.

## v2.7.1 closure

The management route-family refactor, ADMIN credential-promotion repair, and
browser-log handshake filter are released and target-tested. `management.c`
remains only the root-policy, HTTPS-lifecycle, and factory-reset orchestration
boundary. Detailed modules, validation, and release evidence are archived.

## Exact next action

Read [ESP32_DEVELOPMENT_PLAN.md](ESP32_DEVELOPMENT_PLAN.md), inspect only the
UPS-state source needed to reproduce stale values after disconnect, and write a
small acceptance plan before implementation. Hardware interaction requires
fresh authority and [ESP32_PREFLIGHT.md](ESP32_PREFLIGHT.md).
