# ESP32-NUT current development status

This is the fast-start handoff for the active branch. Read it immediately after
[AGENTS.md](../AGENTS.md), then confirm it against live Git. It records only
the active acceptance boundary and one next action; completed evidence belongs
in the archive.

Do not preload `docs/archive/` during a normal startup.

## Repository snapshot

| Field | Current fact |
| --- | --- |
| Active branch | `feature/management-route-families` |
| Base and canonical remote | `main` / `origin/main` |
| Base HEAD when this handoff was updated | `91079a7f5` — resolve live Git before acting |
| Worktree at plan start | Clean |
| Release boundary | Finish the behavior-preserving `management.c` route refactor before beginning the separately scoped `v2.7.1` factory-reset work |
| Target | YD-ESP32-23 / ESP32-S3-WROOM-1-N16R8, ESP-IDF v6.0.2, target `esp32s3` |
| Required services | LAN-only HTTPS `443`; read-only NUT `3493`; retired unauthenticated `8080` remains refused |
| Implementation state | All five remaining handler families and the final route-registration composition are extracted. `management.c` is now 144 lines and contains only root policy, HTTPS startup, and factory reset. ESP-IDF v6.0.2 builds passed after each family and after final composition; source-level handler-body and route-order review passed. |
| Current firmware evidence | The Project Maintainer observed the published v2.7.0-36 build operating with responsive authenticated management, NTP, Wi-Fi, and fresh read-only UPS data. The browser Check Firmware flow also verified that image. The prior OTA failure result remains historical state, not a current device-health failure. |
| Device authority | No flash, OTA install, reset, push, merge, tag, or release is authorized by this handoff. Stop after producing the final candidate firmware and report its path. |

## Active refactoring boundary

The active branch completed only the remaining management route-family
extractions. It did not introduce product behavior or begin factory-reset
work. The resulting structure is:

- `management-status-routes.c` — `GET /api/v1/status`, preserving the
  no-activity session check and exact JSON/status behavior.
- `management-time-routes.c` — the ADMIN-and-CSRF-protected time route.
- `management-token-routes.c` — token list, create, and delete as one API
  family.
- `management-wifi-routes.c` — Wi-Fi scan and configuration as one Wi-Fi
  family.
- `management-ota-routes.c` — browser check/install and bearer Agent install
  as one OTA family.
- `management-routes.c` — the existing 17 route descriptors and registration
  order, extracted after all handler families moved.

`management.c` deliberately remains the small orchestration boundary: root
page policy, HTTPS server lifecycle, and `management_factory_reset()`. Factory
reset stays there until the separately reviewed v2.7.1 slice.

Every moved module needs a narrow header and explicit `src/CMakeLists.txt`
registration. Preserve all exact responses, ADMIN/CSRF/activity policy,
bearer scopes and Authorization-header zeroization, service ports, Wi-Fi
staging/recovery, read-only UPS behavior, and route registration order.

## Validation plan

Run `git diff --check` and an ESP-IDF v6.0.2 `esp32s3` build after each
coherent extraction. Perform source-level route-inventory review as routes are
moved. Produce one final candidate firmware after the complete branch; do not
alter the device until the Project Maintainer separately authorizes upload and
target smoke testing.

## Exact next action

The clean candidate is ready. Stop at its firmware path for explicit upload and
target-test authority; do not alter the device until that authority is given.

## Read only when needed

| Need | Document |
| --- | --- |
| Route order and handler acceptance baseline | [ESP32_ROUTE_INVENTORY.md](ESP32_ROUTE_INVENTORY.md) |
| Detailed route-family plan | [ESP32_REFACTORING_PLAN.md](ESP32_REFACTORING_PLAN.md) |
| 64k continuation packet | [ESP32_64K_AGENT_HANDOFF.md](ESP32_64K_AGENT_HANDOFF.md) |
| Hardware, LAN, COM, flash, or OTA work | [ESP32_PREFLIGHT.md](ESP32_PREFLIGHT.md) |
| Authority for physical, external, or destructive actions | [ESP32_DEVELOPMENT_ROLES.md](ESP32_DEVELOPMENT_ROLES.md) |
| Security boundaries | [ESP32_SECURITY.md](ESP32_SECURITY.md) |

Never record passwords, Wi-Fi credentials, cookies, API tokens, private keys,
or Authorization headers here.
