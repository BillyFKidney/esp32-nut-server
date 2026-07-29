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
| 5 | `management-session.c` — session cookies, setup cookies, idle timeout, CSRF, and login throttling | Planned | Existing ADMIN/CSRF and timeout behavior remains unchanged; no token or password disclosure |
| 6 | `management-http.c` / route composition — shared response helpers, form parsing, and route registration | Planned | All existing routes retain method, path, status, headers, and authorization boundary |
| 7 | `wifi-credentials.c` — active/pending credential persistence and zeroization | Planned | Pending validation, fallback, NVS keys, and factory-reset Wi-Fi erase behavior remain unchanged |
| 8 | `wifi-diagnostics.c` — connection state, DHCP snapshots, and diagnostic messages | Planned | Status/portal diagnostics remain accurate and bounded; no retry or timeout changes |
| 9 | `wifi-recovery.c` — BOOT hold thresholds and restart orchestration | Planned | Three-second Wi-Fi reset and fifteen-second factory reset retain exact thresholds and recovery boundary |

The order is intentionally conservative: logging has no security decision or
network state; status is read-only; certificate and credential extraction then
receive dedicated review before session and route composition; Wi-Fi recovery
is kept separate because it crosses physical input, NVS erasure, and restart.

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

## Branch and validation rules

- Start each stage from the latest `main` unless a documented dependency is
  required and explicitly recorded. `feature/management-credentials-module`
  is temporarily stacked on the local Stage 3 certificate commit, which is
  itself stacked on the local status and logging commits. Rebase each slice
  onto the applicable merged `main` before publishing or opening its pull
  request.
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

After Stage 4 is reviewed and merged, the recommended next slice is
`management-session.c`. It remains deliberately separate because session
cookies, setup cookies, idle timeout, CSRF validation, and login throttling
must preserve the current ADMIN boundary. The later factory-reset slice can
then trace UPS-state retention without mixing certificate, credential, or
session changes.
