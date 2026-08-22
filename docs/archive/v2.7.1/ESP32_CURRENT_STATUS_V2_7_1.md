# ESP32-NUT current development status

This is the fast-start handoff for the active branch. Read it immediately after
[AGENTS.md](../../AGENTS.md), then confirm it against live Git. It records only
the active acceptance boundary and one next action; completed evidence belongs
in the archive.

Do not preload `docs/archive/` during a normal startup.


## Active refactoring boundary

The released v2.7.1 branch completed only the remaining management route-family
extractions plus the validated credential-promotion and browser-log repairs. It
did not begin factory-reset work. The resulting structure is:

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
reset stays there until the separately reviewed final planned v2.7.7 slice.

Every moved module needs a narrow header and explicit `src/CMakeLists.txt`
registration. Preserve all exact responses, ADMIN/CSRF/activity policy,
bearer scopes and Authorization-header zeroization, service ports, Wi-Fi
staging/recovery, read-only UPS behavior, and route registration order.

## Validation plan

Run `git diff --check` and an ESP-IDF v6.0.2 `esp32s3` build after each
coherent extraction. Perform source-level route-inventory review as routes are
moved. The password-change/new-password sign-in and browser-log noise-filter
smoke tests passed on the target. A release build must be produced after the
`v2.7.1` tag is created; it is source-equivalent to the target-tested v2.7.0-41
candidate except for its release identity.

## Exact next action

Create the separately scoped `feature/nut-disconnect-invalidation` branch from
the current `main`, first reproduce the disconnected-UPS stale-value case, and
define the invalidation acceptance boundary. Do not alter the target without
fresh authorization.

## Read only when needed

| Need | Document |
| --- | --- |
| Route order and handler acceptance baseline | [ESP32_ROUTE_INVENTORY_V2_7_1.md](ESP32_ROUTE_INVENTORY_V2_7_1.md) |
| Detailed route-family plan | [ESP32_REFACTORING_PLAN_V2_7_1.md](ESP32_REFACTORING_PLAN_V2_7_1.md) |
| 64k continuation packet | [ESP32_64K_AGENT_HANDOFF_V2_7_1.md](ESP32_64K_AGENT_HANDOFF_V2_7_1.md) |
| Hardware, LAN, COM, flash, or OTA work | [ESP32_PREFLIGHT.md](../ESP32_PREFLIGHT.md) |
| Authority for physical, external, or destructive actions | [ESP32_DEVELOPMENT_ROLES.md](../ESP32_DEVELOPMENT_ROLES.md) |
| Security boundaries | [ESP32_SECURITY.md](../ESP32_SECURITY.md) |

Never record passwords, Wi-Fi credentials, cookies, API tokens, private keys,
or Authorization headers here.
