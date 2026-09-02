# ESP32-NUT v3.0.0 API v2 and token contracts

This is the active planning brief for `feature/api-v2-tokens`. A v3.0 planning
agent must read this document after `AGENTS.md`, `ESP32_CURRENT_STATUS.md`,
and the active development plan. Implementation agents read it only when their
assigned slice concerns v3.0. Do not load the research archive unless a task
below requires its evidence.

## Purpose and release boundary

`v3.0.0` is the intentional compatibility boundary for version 2 of the
management and Agent APIs and their token contracts. It exists so the project
can simplify or remove v1 fields, routes, and token behavior only after the
affected callers, security consequences, resource cost, migration, and
rollback have been reviewed.

This is not a general cleanup release. Every breaking change must have a named
consumer, measured reason, migration behavior, rollback behavior, and
acceptance evidence. The current v1 routes and token behavior remain the
released baseline until v3.0 is authorized, implemented, and accepted.

## Mandatory invariants

- Preserve LAN-only HTTPS `443`, read-only NUT `3493`, and refusal of `8080`.
- Preserve ADMIN-session and CSRF protection for mutations. A read-only route
  must not refresh session activity unless that behavior is explicitly reviewed.
- Keep bearer scopes least-privilege. Never broaden diagnostic or OTA tokens
  merely to preserve a convenience UI behavior.
- Do not disclose passwords, cookies, CSRF values, full API tokens, private
  keys, or Authorization headers in responses, logs, evidence, or chat.
- Keep UPS access read-only. API work does not authorize UPS controls, Wi-Fi
  changes, OTA installation, flashing, reset, publication, or live-device work.

## Required workstreams

1. **Inventory and baseline.** Enumerate every v1 management/Agent route,
   request/response schema, browser caller, command-line helper, authorization
   method, session-activity effect, allocation bound, and known consumer.
   Capture target-independent size/stack/heap baselines before proposing a
   change.
2. **API v2 contract.** Define v2 URI/versioning strategy, response schemas,
   error semantics, cache behavior, compatibility period, v1 retirement rule,
   and the exact client migration path. Keep raw data separate from
   presentation labels.
3. **Token review.** Define v2 token kinds, scopes, issuance/list/delete
   behavior, one-time secret display, persistence, revocation, migration, and
   diagnostic/OTA separation. Existing bearer tokens must not gain access by
   accident.
4. **Resource design.** Replace oversized or repeated response work only with
   measured bounded alternatives. Budget app image growth, internal heap,
   PSRAM, HTTP-task stack, request latency, JSON size, and concurrent browser
   behavior. Do not trade a smaller payload for an unbounded allocation.
5. **Browser and log design.** Keep the device-hosted vanilla UI. Consider a
   dedicated Logs page and a light status API only through v2; retain the
   ADMIN-only full-log endpoint unless a reviewed successor preserves its
   authorization and bounded volatile-retention semantics.
6. **Migration, rollback, and acceptance.** Define mixed v1/v2 behavior,
   token migration/revocation, OTA rollback compatibility, and an evidence
   matrix covering authorization, resource bounds, browser use, services, and
   a post-reboot full NUT poll when target installation is authorized.

## Current decisions and open questions

| Topic | Current direction | Status |
| --- | --- | --- |
| Status logs | Do not remove the six-entry `logs` window from v1. Reconsider it only as an explicitly versioned v2 schema change. | Direction locked for v1; v2 design required. |
| Full logs | `GET /api/v1/admin/logs` is ADMIN-session-only, does not refresh activity, and returns the bounded 24-entry volatile ring oldest-to-newest. | Preserve or provide a reviewed equivalent. |
| UI | Commercial appliance-style vanilla HTML/CSS/JS; no CDN, remote fonts, external icon packs, or framework by default. | Design direction recorded. |
| Memory reporting | Display internal free heap, free PSRAM, and historical minimum free heap as distinct values. | Current v1 semantics understood; v2 naming/shape review required. |
| API/version policy | v3.0 may intentionally break v1 contracts, but only with consumer inventory and migration/rollback evidence. | Must be designed before implementation. |

The v3.0 planner must resolve whether v1 remains available during a migration
period or v3 firmware exposes only v2. Do not infer that choice from the major
firmware number alone.

## Routing

Read [v3.0 API v2 and token research](archive/v3.0.0/ESP32_V3_0_API_V2_TOKENS_RESEARCH.md)
only for these tasks:

- current route/schema and browser-call inventory;
- storage, heap, PSRAM, stack, status-log, or full-log cost analysis;
- UX redesign constraints and dedicated-Logs-page evaluation;
- detailed v1 compatibility, token, and authorization evidence.

Read [ESP32_SECURITY.md](ESP32_SECURITY.md) for any authorization or token
design. Read [ESP32_PREFLIGHT.md](ESP32_PREFLIGHT.md) before any target,
network, flash, reset, or OTA work. The v3.0 slice requires a dedicated
specification before source edits begin.

## Definition of ready for implementation

- A reviewed v2 route and schema inventory names every changed, retained, and
  retired v1 contract and caller.
- Token migration/revocation and v1/v2 coexistence behavior are written and
  approved.
- Resource baselines and per-route budgets are measured on the exact target
  configuration; performance claims are not inferred from source alone.
- Browser, Agent, authorization, rollback, service-boundary, and full-NUT-poll
  acceptance criteria are agreed.
- No live-device or release action is implied by planning approval.
