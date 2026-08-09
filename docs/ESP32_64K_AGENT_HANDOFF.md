# ESP32-NUT 64k-agent handoff

This is the compact continuation packet for an agent with a 64k-token context
window. It is intentionally current, task-focused, and small enough to leave
room for source inspection, edits, build output, and review. Do not preload
the historical archive or the full project conversation.

## Read order

1. [AGENTS.md](../AGENTS.md)
2. [Current status](ESP32_CURRENT_STATUS.md)
3. This handoff
4. The task-specific document from the routing table
5. Only the source files and headers needed by the active slice

Read [ESP32_REFACTORING_PLAN.md](ESP32_REFACTORING_PLAN.md) for management or
Wi-Fi modularization. Read [ESP32_ROUTE_INVENTORY.md](ESP32_ROUTE_INVENTORY.md)
only for route-composition work. Read `ESP32_PREFLIGHT.md` only when hardware,
LAN, COM, flashing, OTA, or recovery work is explicitly authorized. Do not
preload `docs/archive/`.

## Canonical continuation state

- Repository: `BillyFKidney/esp32-nut-server`
- Canonical code base: `main`
- Canonical code HEAD: resolve from live Git; PR #33 merged the refactoring
  stack and handoff into `main` at `22f4cbe6`
- Development device firmware observed after installation:
  `v2.7.0-25-gfffbf96a3`
- Board: YD-ESP32-23 / ESP32-S3-WROOM-1-N16R8
- SDK: ESP-IDF v6.0.2, target `esp32s3`
- Flash: 16 MB DIO at 80 MHz
- PSRAM: 8 MB octal at 80 MHz
- Required services: LAN-only HTTPS `443`, read-only NUT `3493`, refused `8080`

The local `feature/management-route-inventory` checkout is an earlier ancestor
of the refactoring stack now present in `main`. Start the next management
extraction from the live `main` branch.

## Observed device smoke evidence

After the Project Maintainer installed the canonical build, the authenticated
dashboard showed:

- firmware `v2.7.0-25-gfffbf96a3`;
- Wi-Fi connected;
- NUT health `ok` on TCP `3493`;
- UPS status `OL`; and
- the expected UPS and ESP32 hardware diagnostics.

This is target smoke evidence for boot, authentication, Wi-Fi, USB UPS access,
and NUT continuity. It is not full route-matrix acceptance. Do not record
device passwords, cookies, tokens, Authorization headers, or serial numbers.

## Current architecture

On the canonical code base, `src/management.c` is 1,017 lines. The following
boundaries already exist:

- `management-pages.c` — setup, login, cooldown, and administration page HTML;
- `management-auth-routes.c` — setup, login, legacy-login redirect, and ADMIN
  password-change handlers;
- `management-credentials.c` — ADMIN credential format and persistence;
- `management-session.c` — session, setup cookie, CSRF, idle timeout, and
  login throttling state;
- `management-http.c` — defensive headers, responses, JSON, and bounded forms;
- `management-certificates.c` — HTTPS certificate/key persistence;
- `management-status.c` — NUT and hardware data collection; and
- `management-log.c` — bounded in-memory log capture and serialization.

## Current code slice

Create a new code branch from `main`:

```text
feature/management-session-routes
```

Move only the existing session route handlers from `management.c`:

- `POST /logout`; and
- `POST /api/v1/admin/session/activity`.

The extraction must preserve the exact status strings and response bodies, CSRF
checks, session clear/expiry behavior, remaining-session/warning calculation,
and ADMIN session activity behavior. It must not move route registration or
change any route policy. Add the new source explicitly to `src/CMakeLists.txt`
and provide a narrow header interface.

The local ESP-IDF v6.0.2 build passed for this extraction. Target validation
is pending: test authenticated logout and session activity before publishing.
After target validation, follow with time, read-only status response assembly,
token routes, OTA routes, and Wi-Fi routes as separate reviewable boundaries.
Leave factory reset separate until the stale-UPS-state investigation is
complete.

## Validation contract

For every code slice:

1. Preserve the existing HTTPS `443`, NUT `3493`, refused `8080`, ADMIN/CSRF,
   token, certificate, Wi-Fi, and read-only UPS boundaries.
2. Run `git diff --check` and inspect the complete diff.
3. Build with ESP-IDF v6.0.2 for `esp32s3`.
4. Record the exact branch, HEAD, worktree state, build result, and what was
   not target-tested.
5. Do not flash, OTA, push, merge, tag, release, or factory-reset without
   explicit authorization for that action.

The device is currently working. Local compilation does not authorize or
require changing it.
