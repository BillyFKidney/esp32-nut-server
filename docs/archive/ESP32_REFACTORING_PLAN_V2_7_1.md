# ESP32-NUT modular refactoring plan

## Purpose

This plan reduces the responsibility of the large ESP32 management and Wi-Fi
translation units without changing their externally observable behavior. Each
extraction must remain buildable, preserve the current HTTP and NUT interfaces,
and be reviewable as one coherent branch.

This is an engineering-maintenance track. It does not consume a product
version by itself and does not replace the active release acceptance criteria
in [ESP32_DEVELOPMENT_PLAN.md](../ESP32_DEVELOPMENT_PLAN.md).

## Non-negotiable behavior boundaries

Every extraction must preserve:

- LAN-only HTTPS administration on TCP port `443`.
- Read-only NUT service behavior on TCP port `3493`.
- Refusal of the retired unauthenticated TCP port `8080`.
- Existing ADMIN authentication, session, CSRF, and API-token boundaries.
- Self-signed certificate persistence and the documented factory-reset scope
  until the separately reviewed local-CA work begins.
- Wi-Fi credential staging, failed-validation fallback, and physical recovery
  semantics.
- In-memory-only management log capture; logs must not be persisted or contain
  credentials, cookies, tokens, private keys, or Authorization headers.
- Read-only UPS access. This refactoring does not add UPS controls.

The refactoring must not use generated ESP-IDF state or machine-local editor
settings as source inputs. Generated `build/`, `sdkconfig`, managed components,
and dependency locks remain untracked.

## Staged extraction sequence

| Stage | Boundary | Status | Acceptance evidence |
| --- | --- | --- | --- |
| 1 | `management-log.c` — bounded ESP-IDF/NUT log capture and status-response log serialization | Locally committed on `feature/management-log-module`; review pending | ESP-IDF v6.0.2 target build passes; public callers use `management-log.h`; status JSON and log behavior are unchanged by inspection; no host test harness is configured for this component |
| 2 | `management-status.c` — NUT and hardware diagnostic snapshots | Implemented on `feature/management-status-module`; review pending | ESP-IDF v6.0.2 target build passes; NUT dstate and hardware diagnostics moved behind `management-status.h`; authenticated route, session check, and response fields remain in `management.c` |
| 3 | `management-certificates.c` — certificate/key loading, generation, and NVS persistence | Implemented on `feature/management-certificates-module`; review pending | ESP-IDF v6.0.2 target build passes; HTTPS startup receives the same material through `management-certificates.h`; missing/incomplete blobs still regenerate; private key handling remains zeroized |
| 4 | `management-credentials.c` — ADMIN credential storage, PBKDF2 verification, and legacy migration | Implemented on `feature/management-credentials-module`; review pending | ESP-IDF v6.0.2 target build passes; setup, login, password change, migration, and factory-reset credential erasure retain current outcomes by code-path inspection |
| 5 | `management-session.c` — session cookies, setup cookies, idle timeout, CSRF, and login throttling | Implemented on `feature/management-session-module`; review pending | ESP-IDF v6.0.2 target build passes; existing ADMIN/CSRF, timeout, and cooldown behavior remains unchanged by code-path inspection; no token or password disclosure |
| 6 | `management-http.c` — shared response helpers, JSON utilities, and bounded form handling | Implemented on `feature/management-http-module`; review pending | ESP-IDF v6.0.2 target build passes; headers, content types, Content Security Policy, body limit, decoding, and zeroization remain unchanged by code-path inspection; routes and authorization remain in `management.c` |
| 7 | `wifi-credentials.c` — active/pending credential persistence | Implemented on `feature/wifi-credentials-module`; review pending | ESP-IDF v6.0.2 target build passes; the four `wifi-config` keys, pending-record erase, full namespace erase, and caller zeroization remain unchanged by code-path inspection; connection and reset behavior remain in `wifi.c` |
| 8 | `wifi-diagnostics.c` — connection diagnostic and read-only DHCP snapshots | Implemented on `feature/wifi-diagnostics-module`; review pending | ESP-IDF v6.0.2 target build passes; message text and DHCP state checks remain unchanged by code-path inspection; retries, timeouts, portal behavior, and recovery remain in `wifi.c` |
| 9 | `wifi-provisioning-web.c` — captive-portal HTTP routes, form decode, JSON responses, and route registration | Implemented on `feature/wifi-provisioning-web-module`; target smoke validation partially passed; review pending | ESP-IDF v6.0.2 target build passes. The combined local candidate was installed through the authenticated browser OTA path, rebooted, and accepted an existing ADMIN password; temporary portal endpoints remain untested because the configured development board was not deliberately placed into recovery mode |
| 10 | `wifi-recovery.c` — BOOT hold thresholds and restart orchestration | Planned | Three-second Wi-Fi reset and fifteen-second factory reset retain exact thresholds and recovery boundary |
| 11a | `management-status-routes.c` — authenticated status response assembly | Implemented on `feature/management-route-families`; ESP-IDF v6.0.2 build passed | Preserves `management_require_session_without_activity()`, status/JSON field behavior, and log/status snapshot semantics |
| 11b | `management-time-routes.c` — ADMIN time configuration | Implemented on `feature/management-route-families`; ESP-IDF v6.0.2 build passed | Preserves ADMIN/CSRF policy, form handling, NVS/time behavior, responses, and manual/NTP behavior |
| 11c | `management-token-routes.c` — list/create/delete API-token family | Implemented on `feature/management-route-families`; ESP-IDF v6.0.2 build passed | Preserves list activity semantics, creation/deletion ADMIN/CSRF policy, token scope behavior, and response bodies |
| 11d | `management-wifi-routes.c` — Wi-Fi scan and configuration family | Implemented on `feature/management-route-families`; ESP-IDF v6.0.2 build passed | Preserves scan/configuration authorization, scan responses, credential staging, restart scheduling, and zeroization |
| 11e | `management-ota-routes.c` — browser and Agent OTA family | Implemented on `feature/management-route-families`; ESP-IDF v6.0.2 build passed | Preserves browser ADMIN/CSRF, bearer `ota.install` scope, Authorization-header zeroization, OTA response policy, inactive-slot behavior, and restart coordination |
| 12 | `management-routes.c` — route composition after all handler families move | Implemented on `feature/management-route-families`; ESP-IDF v6.0.2 build passed | [ESP32_ROUTE_INVENTORY_V2_7_1.md](ESP32_ROUTE_INVENTORY_V2_7_1.md) records all 17 routes and source-level guards; paths, methods, status codes, authorization checks, response payload semantics, capacity assertion, and registration order remain unchanged |

The order is intentionally conservative: logging has no security decision or
network state; status is read-only; certificate and credential extraction then
receive dedicated review before session and route composition; Wi-Fi recovery
is kept separate because it crosses physical input, NVS erasure, and restart.

The staged management-route work, the credential-promotion repair, and the
browser-log noise filter were published in `v2.7.1`. The historical branch
labels in the table identify their original review boundaries; they are not
active work.

## Management module inventory (corrected)

The following inventory supersedes shorthand task lists that combined distinct
responsibilities. "Complete" means the focused source boundary exists and has
passed its recorded local build; it does not mean every related route has moved
out of `management.c`.

| Responsibility | Focused module | Current status | Deliberately retained in `management.c` or later work |
| --- | --- | --- | --- |
| HTTPS server bootstrap and route registration | `management-routes.c` after handler-family extractions | Active plan; the route inventory is complete | HTTPS server configuration remains in `management.c`; route order and all 17 registrations move only in the final composition extraction |
| ADMIN credential format, PBKDF2 verification, persistence, legacy migration | `management-credentials.c` | Complete | Setup, login, password-change, and factory-reset route policy |
| ADMIN cookies, CSRF, idle timeout, and login throttling | `management-session.c` | Complete | Route-level authorization, 401/403/429 response selection, and form handling |
| TLS certificate/key storage and self-signed material lifecycle | `management-certificates.c` | Complete | HTTPS server startup and route registration |
| NUT and hardware diagnostic snapshots | `management-status.c` | Complete | Authenticated status-route authorization and final JSON response aggregation |
| Bounded in-memory management-log capture and status serialization | `management-log.c` | Complete | No separate log API exists or is planned in this extraction |
| OTA body handling, image verification, descriptor identity, boot selection, and restart coordination | `ota.c` | Complete focused boundary | `management-ota-routes.c` will own browser and Agent route policy while `ota.c` retains OTA mechanics |
| Common HTTP headers, HTML/JSON/redirect responses, JSON utilities, and bounded form handling | `management-http.c` | Complete | It is not an HTML-template module and does not own route decisions |
| Setup, login, cooldown, and authenticated administration page rendering | `management-pages.c` | Implemented on `feature/management-pages-module`; ESP-IDF v6.0.2 build passed; target behavior not yet tested for this refactor | Root-page ADMIN/session/CSRF decisions remain in `management.c`; the page module does not own route handlers or policy |
| Setup/login/password route handlers | `management-auth-routes.c` | Implemented on `feature/management-auth-routes`; ESP-IDF v6.0.2 build passed; development-device smoke test passed after installation | Root-page authorization, logout, server startup, route order, and all non-auth routes remain in `management.c` |
| Shared authorization helpers | `management-authorization.c` | Merged to `main` by PR #35; ESP-IDF v6.0.2 build passed | Session gates, bearer scope, unauthorized responses, header zeroization, and activity semantics remain unchanged by code-path inspection |
| Remaining status, time, token, OTA, and Wi-Fi route handlers | Five focused route-family modules | Implemented on `feature/management-route-families`; local builds passed | Cohesive families, not individual operations; preserve each route's documented authorization and response semantics |
| Final route composition | `management-routes.c` | Implemented on `feature/management-route-families`; local build passed | Preserves route order, capacity assertion, headers, authorization, and endpoint semantics; HTTPS lifecycle remains in `management.c` |

The completed page-rendering, ADMIN-route, shared-authorization, and session
route extractions reduce `management.c` without changing security decisions.
The active branch completed the remaining handler families and the final
route-composition move. It did not begin the separately scoped final planned
v2.7.7 factory-reset investigation.

### Current management page-rendering slice

`management-pages.c` owns only the existing setup, login, login-cooldown, and
authenticated administration HTML/JavaScript rendering. The page module is
passed the request and a caller-provided CSRF value; it does not inspect or
mutate credentials, sessions, cookies, API tokens, NVS, Wi-Fi state, or route
registration.

`management.c` continues to own first-run setup-session creation, root-page
ADMIN authorization, login-throttle decisions, and CSRF copying/zeroization.
The page module owns no route decision. The later `management-auth-routes.c`
slice owns the four setup/login/password handler bodies while retaining their
existing response policy. The original page-template payloads were moved
byte-for-byte by source comparison, apart from the renamed private page-buffer
constant. The resulting ESP-IDF v6.0.2 build passed. A target browser check for
this refactor remains separate and requires explicit authorization; no firmware
installation is implied by the local build.

### Current ADMIN route-handler slice

`management-auth-routes.c` owns the existing first-run setup, sign-in,
legacy-login redirect, and ADMIN password-change handlers. It depends only on
the existing credential, session, page-rendering, and HTTP helper interfaces.
The route paths, methods, form-size limit, login-cooldown behavior, response
statuses/messages, credential migration behavior, and zeroization of password,
confirmation, form-body, and CSRF buffers remain unchanged.

`management.c` retains its root-page security decision, logout handler,
server startup, route-registration order, and all non-auth handlers. The four
existing route registrations now reference exported handler symbols from the
focused module. Source comparison confirms the moved bodies are unchanged
apart from those symbols, and an ESP-IDF v6.0.2 build passed. Because these
are security-sensitive browser flows, target acceptance must be separately
authorized and includes setup, login failure, cooldown, password change, and
logout checks. No device-side install is implied by this local refactor.

### Stage 3: HTTPS certificate material

`management-certificates.c` exclusively owns the persisted self-signed HTTPS
certificate and private-key lifecycle. It keeps the existing `management` NVS
namespace and the `https-cert` and `https-key` blob keys. On startup it loads
both blobs; if either is missing, empty, or unreadable, it zeroizes any
in-memory private-key data and generates and persists a replacement pair.

The generated pair deliberately retains the previous security behavior: an EC
P-256 key, a device-specific `ESP32-NUT-…` common name based on the station
MAC, certificate serial generation, and the existing validity, basic-constraint,
and key-usage settings. The HTTPS server remains responsible for service
startup and route registration; it only consumes the module-owned material via
the read-only [include/management-certificates.h](../../include/management-certificates.h)
interface. No certificate or private-key content is logged or documented.

### Stage 4: ADMIN credentials

`management-credentials.c` exclusively owns the ADMIN credential format,
PBKDF2-HMAC-SHA-256 derivation, constant-time hash comparison, NVS persistence,
and legacy salt/hash migration detection. It preserves the current
`management` namespace, `admin-cred` current-format blob, and `admin-salt` /
`admin-hash` legacy blobs; a successful legacy login still signals the route to
rewrite the current credential format. Credential structures and temporary
salt/hash buffers retain their existing zeroization behavior.

The HTTPS routes remain responsible for form parsing, CSRF checks, session
creation, login throttling, migration logging, and response selection. Factory
reset remains in `management.c` and erases the full `management` namespace, so
credential-reset behavior is unchanged. Passwords, derived hashes, and tokens
must never be logged or documented.

### Stage 5: ADMIN session and CSRF state

`management-session.c` exclusively owns the in-memory ADMIN session cookie,
CSRF value, first-run setup cookie, constant-time token comparison, idle-time
tracking, and failed-login cooldown accounting. It preserves the existing token
length, Secure/HttpOnly/SameSite cookie attributes, five-minute setup-cookie
lifetime, fifteen-minute ADMIN idle timeout, five-minute warning threshold,
five-failure threshold, and sixty-second cooldown.

At the session-state boundary, `management.c` retained route-level policy,
including the existing 401/403/429 output, form parsing, bearer-token checks,
and JSON assembly. The later, separately reviewed
`management-auth-routes.c` slice moves only the setup/login/password handlers;
all other route families remain in `management.c`. Session authorization and
CSRF decisions stay in `management-session.c`, which keeps the ADMIN boundary
independently traceable. No cookie, CSRF value, password, token, or
Authorization header is logged or documented.

### Stage 6: shared HTTPS response and bounded form handling

`management-http.c` exclusively owns the existing defensive response headers,
HTML/JSON/redirect send mechanics, Content Security Policy, bounded JSON
appending and escaping, and bounded `application/x-www-form-urlencoded` body
read and decode helpers. It preserves the existing 640-byte body limit, timeout
and size errors, URL-decoding behavior, and zeroization of the temporary copied
form body.

At the shared HTTP-helper boundary, `management.c` retained route path/method,
authorization/CSRF decisions, bearer-token handling, JSON field order, and
route registration. Later focused modules may own a whole handler family while
preserving those endpoint semantics. Route composition remains a distinct later
stage because it requires a route-by-route acceptance inventory. No password,
cookie, CSRF value, token, or Authorization header is logged or documented.

### Stage 7: Wi-Fi active and pending credential persistence

`wifi-credentials.c` exclusively owns the existing `wifi-config` NVS namespace
and its four records: active `ssid`/`password` and pending `pending-ssid`/
`pending-pass`. It preserves the active/pending record format, NVS commit
points, idempotent pending-key erase handling, and full namespace erase used by
the physical Wi-Fi reset. Callers continue to zeroize their own credential
buffers after use.

`wifi.c` retains global NVS initialization, user-input validation, pending
record staging and restart scheduling, pending-record removal before testing,
promotion after connection success, fallback to the active network, portal
behavior, connection retries, BOOT thresholds, and factory-reset coordination.
No SSID, password, or credential record is logged or documented.

### Stage 8: Wi-Fi diagnostics and read-only DHCP snapshots

`wifi-diagnostics.c` exclusively owns the bounded user-facing connection
diagnostic, its synchronization, the DHCP snapshot structure, and existing
state-to-message formatting. The station interface is supplied explicitly to
the read-only snapshot call; the existing DHCP states, offer checks, messages,
and 192-byte diagnostic capacity remain unchanged.

`wifi.c` retains association tracking, DHCP startup, event handling, retry and
timeout decisions, portal status response composition, connection setup, and
physical recovery. The diagnostic module must not call `esp_wifi_connect`,
`esp_wifi_disconnect`, `esp_restart`, or alter event-group state.

### Stage 9: Wi-Fi provisioning web

`wifi-provisioning-web.c` owns the existing temporary captive-portal HTTP
surface only: shared portal response headers, JSON escaping, bounded
`application/x-www-form-urlencoded` decoding, network-list serialization,
credential-stage form handling, and registration of the four existing routes.
It preserves `GET /`, `GET /api/networks`, `POST /api/configure`, and `GET
/api/status`, including their status codes, response text, 256-byte request-body
limit, cache/content-security headers, and portal-only HTTP service scope.

`wifi.c` retains Wi-Fi radio initialization, station connection and retry state,
active/pending credential lifecycle, BOOT-button recovery, SoftAP setup,
captive-DNS lifecycle, portal scheduling, and HTTP-server handle ownership.
The focused web module receives only the existing connection-request flag and a
restart-scheduling callback; it does not decide connection retries, erase
credentials, configure the AP/DNS service, or invoke factory reset.

Target acceptance requires intentionally entering the temporary portal on a
non-production device or during an explicitly approved recovery session, then
checking the four routes. Do not submit real Wi-Fi credentials merely to test
this refactor; use invalid input or a separately authorized credential-staging
test.

### Stages 11–12: remaining route families and route composition

[ESP32_ROUTE_INVENTORY_V2_7_1.md](ESP32_ROUTE_INVENTORY_V2_7_1.md) records the 17 existing
HTTPS route registrations in their current order, together with source-level
authorization and CSRF guards. The completed `management-routes.c` extraction
moves the descriptors and registration loop only after the status, time, token,
Wi-Fi, and OTA handler families became external. It preserves the inventory
exactly. The applied family order was status; time; tokens; Wi-Fi; OTA; then
route composition. This keeps each source unit small enough for constrained
agent context without creating per-operation files that obscure a cohesive API
or inflate boilerplate.

## Completed extraction summary: management log module

The first slice moves the following private state and behavior out of
`src/management.c`:

- bounded ring-buffer entries and pending-line assembly;
- ESP-IDF `vprintf` chaining and NUT syslog capture;
- timestamp formatting and log-level normalization;
- JSON escaping and appending of the most recent log entries.

The dedicated interface is [include/management-log.h](../../include/management-log.h).
The existing callers are `src/main.c`, `lib/syslog/syslog.c`, and the
authenticated management status handler. Focused modules are listed explicitly
in `src/CMakeLists.txt` because ESP-IDF does not reconfigure the legacy source
glob when a new file appears during an incremental build. The component
registration was rechecked by the target build.

No route, response field, log capacity, timestamp format, or persistence
behavior is intentionally changed.

## Completed extraction summary: management status module

The second slice moves only read-only state collection out of
`src/management.c`:

- normalized NUT dstate reads, including stale/unavailable handling and
  manufacturer/model/serial fallback;
- board profile, module profile, flash, PSRAM, heap, and optional chip
  temperature diagnostics;
- idempotent hardware-diagnostic initialization used before HTTPS startup and
  by the authenticated status route.

The route's session check, HTTP response helper, JSON field order, response
size limit, and error response remain in `src/management.c`. This keeps the
ADMIN boundary unchanged while making the runtime data sources independently
traceable for the factory-reset investigation.

## Completed extraction summary: Wi-Fi provisioning web module

This slice moves the temporary captive-portal web handling out of `src/wifi.c`:

- common portal response headers and JSON response mechanics;
- bounded JSON escaping and URL-form decoding;
- the portal root, network-list, status, and credential-configuration
  handlers; and
- explicit registration of those four temporary HTTP routes.

The dedicated interface is
[include/wifi-provisioning-web.h](../../include/wifi-provisioning-web.h). The
orchestrator supplies its existing connection-request state and restart task
through a narrow context; neither Wi-Fi radio state nor the lifetime of the
temporary HTTP service moves out of `wifi.c`. The new source is explicitly
listed in `src/CMakeLists.txt` so an incremental ESP-IDF build includes it.

No provisioning route, portal header, response text, request-body limit,
credential-validation rule, or connection/recovery behavior is intentionally
changed. The module does not log credentials.

## 64k-agent continuation boundary

The historical [ESP32_64K_AGENT_HANDOFF_V2_7_1.md](ESP32_64K_AGENT_HANDOFF_V2_7_1.md)
records the then-preferred startup packet for a context-limited agent. The
canonical continuation base was current `main`, which contained the merged
refactoring stack and handoff from PR #33. The older
`feature/management-route-inventory` checkout was an ancestor and was not the
next code base by default.

The active code slice is intentionally narrow: move only the existing logout
and session-activity handlers from `management.c` into
`management-session-routes.c`. Do not combine that work with route
registration, other route-handler movement, factory reset, Wi-Fi recovery, or
behavior changes. The development device has already received and smoke-tested
the preceding build; this extraction now requires separately authorized target
validation before publishing.

## Branch and validation rules

- Start each stage from the latest `main` unless a documented dependency is
  required and explicitly recorded. `feature/wifi-provisioning-web-module` is
  temporarily stacked on the local route-inventory and Stage 8 diagnostic
  commits, which are themselves stacked on the local Wi-Fi credential,
  HTTP-helper, session, credential, certificate, status, and logging commits.
  Rebase each slice onto the applicable merged `main` before publishing or
  opening its pull request.
- Use one branch and pull request per stage; do not combine logging, status,
  authentication, certificates, and Wi-Fi recovery in one change.
- Run `git diff --check` and inspect the complete diff before building.
- Build with ESP-IDF v6.0.2 for target `esp32s3` after every extraction.
- Run available host tests when the repository build exposes them; record when
  a module is target-only and therefore not host-tested.
- Hardware, HTTPS, NUT, and browser validation remain separate acceptance work;
  a successful compile does not prove those behaviors.
- Do not push, merge, tag, release, flash, OTA, or factory-reset without the
  authority required by the current status and roles documents.

## Next extractions

The Wi-Fi provisioning web module is locally implemented and built, but still
needs focused temporary-portal acceptance on a non-production device or during
an explicitly approved recovery session. Do not extract `wifi-recovery.c`
until a target-side recovery test plan is approved: it owns physical BOOT input,
credential erasure, management reset, and restart. Before a
`management-routes.c` extraction, execute the route inventory's target-side
acceptance matrix. The later factory-reset slice can then trace UPS-state
retention without mixing certificate, credential, session, response-helper, or
Wi-Fi state changes.
