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
| Implementation state | All five remaining handler families and the final route-registration composition are extracted. `management.c` is now 144 lines and contains only root policy, HTTPS startup, and factory reset. Target testing exposed and then resolved an ADMIN-password change lockout: new credentials now stage, persist, re-read, and verify before promotion. The next diagnostic-only change suppresses the routine ESP-IDF HTTPS handshake INFO line from the bounded browser log snapshot, while retaining it on serial and retaining TLS errors. |
| Current firmware evidence | The Project Maintainer observed v2.7.0-40 operating with responsive authenticated management, NTP, Wi-Fi, fresh read-only UPS data, Wi-Fi scan/configuration, token issuance/revocation, and two successful ADMIN password rotations followed by a new-password sign-in. The dashboard log currently fills with routine HTTPS handshake messages; the active diagnostic-only change addresses that display noise. |
| Device authority | The full erase and flash were explicitly authorized and completed for lockout recovery. Do not perform another flash, OTA install, reset, push, merge, tag, or release without a new explicit request. |

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
moved. The password-change and new-password sign-in smoke test passed on the
target. Build the diagnostic log-noise filter, then confirm that the browser
log snapshot shows actionable events rather than routine HTTPS handshakes. Do
not alter the device until the Project Maintainer separately authorizes upload
and target smoke testing.

## Exact next action

Commit the diagnostic log-noise filter, reconfigure the ESP-IDF build for its
clean Git version, and stop at that candidate's firmware path for explicit
upload and one browser-log snapshot check.

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
