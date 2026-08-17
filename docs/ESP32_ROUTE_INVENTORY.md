# HTTPS management route inventory

This is the source-level acceptance baseline for any future route-composition
refactor. It was derived from `src/management.c` on
`feature/management-route-inventory`; it is **not** runtime validation.

## Invariants

- Keep HTTPS LAN-only on TCP `443` and leave retired TCP `8080` refused.
- Keep every path, HTTP method, registration order, handler outcome, and
  authorization boundary unchanged unless a separately approved feature says
  otherwise.
- Do not expose cookies, CSRF values, passwords, API tokens, private keys, or
  Authorization headers in logs, test fixtures, or documentation.
- Do not move physical BOOT recovery, NVS erase timing, factory reset, or OTA
  behavior into this slice.

## Current registration order

| # | Method | Path | Current source guard / purpose |
| ---: | --- | --- | --- |
| 1 | `GET` | `/` | State-dependent: first-run setup, login page, or ADMIN dashboard |
| 2 | `POST` | `/setup` | First-run setup cookie and CSRF validation |
| 3 | `GET` | `/login` | Login-page redirect behavior |
| 4 | `POST` | `/login` | ADMIN credential verification and throttling |
| 5 | `POST` | `/api/v1/admin/password` | ADMIN session plus CSRF validation |
| 6 | `POST` | `/logout` | ADMIN session plus CSRF validation |
| 7 | `GET` | `/api/v1/status` | ADMIN session without activity refresh |
| 8 | `POST` | `/api/v1/admin/session/activity` | ADMIN session plus CSRF validation |
| 9 | `POST` | `/api/v1/admin/time` | ADMIN session plus CSRF validation |
| 10 | `POST` | `/api/v1/ota/check` | ADMIN session plus CSRF validation |
| 11 | `POST` | `/api/v1/ota/install` | ADMIN session plus CSRF validation |
| 12 | `GET` | `/api/v1/admin/tokens` | ADMIN session |
| 13 | `POST` | `/api/v1/admin/tokens` | ADMIN session plus CSRF validation |
| 14 | `DELETE` | `/api/v1/admin/tokens` | ADMIN session plus CSRF validation |
| 15 | `GET` | `/api/v1/admin/wifi/scan` | ADMIN session |
| 16 | `POST` | `/api/v1/admin/wifi` | ADMIN session plus CSRF validation |
| 17 | `POST` | `/api/v1/agent/ota/install` | Bearer token with `ota.install` scope |

The existing route-capacity assertion limits this list to
`MANAGEMENT_HTTPS_ROUTE_CAPACITY`; moving the array must retain that protection.

## Approved extraction map

The active `feature/management-route-families` branch uses cohesive route
families rather than one file per URI:

| Module | Routes |
| --- | --- |
| `management-status-routes.c` | `GET /api/v1/status` |
| `management-time-routes.c` | `POST /api/v1/admin/time` |
| `management-token-routes.c` | token list/create/delete |
| `management-wifi-routes.c` | Wi-Fi scan/configuration |
| `management-ota-routes.c` | browser OTA check/install and bearer Agent install |
| `management-routes.c` | all existing descriptors and registration order; final extraction only |

The existing authentication and session route modules remain cohesive and are
not subdivided. `management.c` retains root-page policy, HTTPS server
lifecycle, and factory reset; factory reset belongs to the separate v2.7.1
slice.

## Required preflight before Stage 10 code changes

1. Confirm the live branch, working tree, device IP, and USB path rather than
   relying on historical values.
2. Review each handler against this table and preserve its path, method,
   authorization helper, CSRF condition, response status selection, and
   response payload semantics.
3. Build with ESP-IDF v6.0.2 for `esp32s3`.
4. When authorized for target testing, validate setup, login, logout, password
   change, idle-session behavior, status, token operations, Wi-Fi scan/stage,
   time configuration, browser OTA check/install, and bearer-token OTA denial
   and acceptance behavior.
5. Do not publish, merge, flash, OTA, factory-reset, tag, or release without
   the authority required by the current status and roles documents.

## Related documents

- [Current status](ESP32_CURRENT_STATUS.md)
- [Refactoring plan](ESP32_REFACTORING_PLAN.md)
- [Preflight](ESP32_PREFLIGHT.md)
- [Security boundaries](ESP32_SECURITY.md)
- [Roles and authority](ESP32_DEVELOPMENT_ROLES.md)
