# ESP32-NUT modular refactoring plan

## Purpose

This plan reduces the responsibility of the large ESP32 management and Wi-Fi
translation units without changing their externally observable behavior. Each
extraction must remain buildable, preserve the current HTTP and NUT interfaces,
and be reviewable as one coherent branch.

This is an engineering-maintenance track. It does not consume a product
version by itself and does not replace the active release acceptance criteria
in [ESP32_DEVELOPMENT_PLAN.md](ESP32_DEVELOPMENT_PLAN.md).

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
| 11 | `management-routes.c` — route composition after route-by-route behavior inventory | Inventory recorded on `feature/management-route-inventory`; code extraction planned | [ESP32_ROUTE_INVENTORY.md](ESP32_ROUTE_INVENTORY.md) records all 17 routes and source-level guards; paths, methods, status codes, authorization checks, response payload semantics, and registration order remain unchanged |

The order is intentionally conservative: logging has no security decision or
network state; status is read-only; certificate and credential extraction then
receive dedicated review before session and route composition; Wi-Fi recovery
is kept separate because it crosses physical input, NVS erasure, and restart.

## Management module inventory (corrected)

The following inventory supersedes shorthand task lists that combined distinct
responsibilities. "Complete" means the focused source boundary exists and has
passed its recorded local build; it does not mean every related route has moved
out of `management.c`.

| Responsibility | Focused module | Current status | Deliberately retained in `management.c` or later work |
| --- | --- | --- | --- |
| HTTPS server bootstrap and route registration | None yet; eventual `management-server.c` and/or `management-routes.c` | Planned; the route inventory is complete | HTTPS server configuration, route order, and all 17 registrations |
| ADMIN credential format, PBKDF2 verification, persistence, legacy migration | `management-credentials.c` | Complete | Setup, login, password-change, and factory-reset route policy |
| ADMIN cookies, CSRF, idle timeout, and login throttling | `management-session.c` | Complete | Route-level authorization, 401/403/429 response selection, and form handling |
| TLS certificate/key storage and self-signed material lifecycle | `management-certificates.c` | Complete | HTTPS server startup and route registration |
| NUT and hardware diagnostic snapshots | `management-status.c` | Complete | Authenticated status-route authorization and final JSON response aggregation |
| Bounded in-memory management-log capture and status serialization | `management-log.c` | Complete | No separate log API exists or is planned in this extraction |
| OTA body handling, image verification, descriptor identity, boot selection, and restart coordination | `ota.c` | Complete focused boundary | Management routes retain ADMIN/CSRF/content-type checks and response policy; a separate `management-ota.c` is not currently needed |
| Common HTTP headers, HTML/JSON/redirect responses, JSON utilities, and bounded form handling | `management-http.c` | Complete | It is not an HTML-template module and does not own route decisions |
| Setup, login, cooldown, and authenticated administration page rendering | `management-pages.c` | Implemented on `feature/management-pages-module`; ESP-IDF v6.0.2 build passed; target behavior not yet tested for this refactor | ADMIN/session/CSRF decisions, route handlers, and response policy |
| Setup/login/password route handlers | Future `management-auth-routes.c` only if still beneficial after pages move | Planned | Current behavior remains in `management.c` until a small, separately reviewed slice |
| Wi-Fi, token, and time route handlers | Future focused route modules only where a cohesive boundary remains | Planned | Current behavior remains in `management.c` until separately reviewed |
| Final route composition | `management-routes.c` and/or `management-server.c` | Planned after route acceptance matrix | Preserve route order, headers, authorization, and endpoint semantics exactly |

The next completed page-rendering extraction will reduce `management.c` without
moving security decisions. It is intentionally safer than extracting setup or
login routes immediately after the request-header-limit recovery work.

### Current management page-rendering slice

`management-pages.c` owns only the existing setup, login, login-cooldown, and
authenticated administration HTML/JavaScript rendering. The page module is
passed the request and a caller-provided CSRF value; it does not inspect or
mutate credentials, sessions, cookies, API tokens, NVS, Wi-Fi state, or route
registration.

`management.c` continues to own first-run setup-session creation, ADMIN
authorization, login-throttle decisions, CSRF copying and zeroization, all
route handlers, and every success/error response policy. The original
page-template payloads were moved byte-for-byte by source comparison, apart
from the renamed private page-buffer constant. The resulting ESP-IDF v6.0.2
build passed. A target browser check for this refactor remains separate and
requires explicit authorization; no firmware installation is implied by the
local build.

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
the read-only [include/management-certificates.h](../include/management-certificates.h)
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

`management.c` retains every route handler and its response policy, including
the existing 401/403/429 output, form parsing, bearer-token checks, and JSON
assembly. It calls the module for authorization and CSRF decisions, which keeps
the ADMIN boundary unchanged while making the mutable state independently
traceable. No cookie, CSRF value, password, token, or Authorization header is
logged or documented.

### Stage 6: shared HTTPS response and bounded form handling

`management-http.c` exclusively owns the existing defensive response headers,
HTML/JSON/redirect send mechanics, Content Security Policy, bounded JSON
appending and escaping, and bounded `application/x-www-form-urlencoded` body
read and decode helpers. It preserves the existing 640-byte body limit, timeout
and size errors, URL-decoding behavior, and zeroization of the temporary copied
form body.

`management.c` retains each route handler, route path and method, response
status selection, authorization and CSRF decision, form-field semantics,
bearer-token handling, JSON field order, and route registration. Route
composition remains a distinct later stage because it requires a route-by-route
acceptance inventory. No password, cookie, CSRF value, token, or Authorization
header is logged or documented.

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

### Stage 11: route-composition inventory

[ESP32_ROUTE_INVENTORY.md](ESP32_ROUTE_INVENTORY.md) records the 17 existing
HTTPS route registrations in their current order, together with source-level
authorization and CSRF guards. It is documentation only: no handler,
registration, header, response, or device behavior has been moved. A future
`management-routes.c` extraction must first complete its target-side acceptance
matrix and preserve the inventory exactly.

## Current slice: management log module

The first slice moves the following private state and behavior out of
`src/management.c`:

- bounded ring-buffer entries and pending-line assembly;
- ESP-IDF `vprintf` chaining and NUT syslog capture;
- timestamp formatting and log-level normalization;
- JSON escaping and appending of the most recent log entries.

The dedicated interface is [include/management-log.h](../include/management-log.h).
The existing callers are `src/main.c`, `lib/syslog/syslog.c`, and the
authenticated management status handler. Focused modules are listed explicitly
in `src/CMakeLists.txt` because ESP-IDF does not reconfigure the legacy source
glob when a new file appears during an incremental build. The component
registration was rechecked by the target build.

No route, response field, log capacity, timestamp format, or persistence
behavior is intentionally changed.

## Current slice: management status module

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

## Current slice: Wi-Fi provisioning web module

This slice moves the temporary captive-portal web handling out of `src/wifi.c`:

- common portal response headers and JSON response mechanics;
- bounded JSON escaping and URL-form decoding;
- the portal root, network-list, status, and credential-configuration
  handlers; and
- explicit registration of those four temporary HTTP routes.

The dedicated interface is
[include/wifi-provisioning-web.h](../include/wifi-provisioning-web.h). The
orchestrator supplies its existing connection-request state and restart task
through a narrow context; neither Wi-Fi radio state nor the lifetime of the
temporary HTTP service moves out of `wifi.c`. The new source is explicitly
listed in `src/CMakeLists.txt` so an incremental ESP-IDF build includes it.

No provisioning route, portal header, response text, request-body limit,
credential-validation rule, or connection/recovery behavior is intentionally
changed. The module does not log credentials.

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
