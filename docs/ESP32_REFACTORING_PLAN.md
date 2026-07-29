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
| 1 | `management-log.c` — bounded ESP-IDF/NUT log capture and status-response log serialization | Implemented on `feature/management-log-module`; review pending | ESP-IDF v6.0.2 target build passes; public callers use `management-log.h`; status JSON and log behavior are unchanged by inspection; no host test harness is configured for this component |
| 2 | `management-status.c` — NUT and hardware diagnostic snapshots | Planned | Authenticated `/api/v1/status` response is byte/field-equivalent for unchanged inputs; stale/unavailable semantics remain explicit |
| 3 | `management-certificates.c` — certificate/key loading, generation, and NVS persistence | Planned | HTTPS 443 starts with the same stored material; missing/incomplete blobs still regenerate; private key handling remains zeroized |
| 4 | `management-credentials.c` — ADMIN credential storage, PBKDF2 verification, and legacy migration | Planned | Setup, login, password change, migration, and factory-reset credential erasure retain current outcomes |
| 5 | `management-session.c` — session cookies, setup cookies, idle timeout, CSRF, and login throttling | Planned | Existing ADMIN/CSRF and timeout behavior remains unchanged; no token or password disclosure |
| 6 | `management-http.c` / route composition — shared response helpers, form parsing, and route registration | Planned | All existing routes retain method, path, status, headers, and authorization boundary |
| 7 | `wifi-credentials.c` — active/pending credential persistence and zeroization | Planned | Pending validation, fallback, NVS keys, and factory-reset Wi-Fi erase behavior remain unchanged |
| 8 | `wifi-diagnostics.c` — connection state, DHCP snapshots, and diagnostic messages | Planned | Status/portal diagnostics remain accurate and bounded; no retry or timeout changes |
| 9 | `wifi-recovery.c` — BOOT hold thresholds and restart orchestration | Planned | Three-second Wi-Fi reset and fifteen-second factory reset retain exact thresholds and recovery boundary |

The order is intentionally conservative: logging has no security decision or
network state; status is read-only; certificate and credential extraction then
receive dedicated review before session and route composition; Wi-Fi recovery
is kept separate because it crosses physical input, NVS erasure, and restart.

## Current slice: management log module

The first slice moves the following private state and behavior out of
`src/management.c`:

- bounded ring-buffer entries and pending-line assembly;
- ESP-IDF `vprintf` chaining and NUT syslog capture;
- timestamp formatting and log-level normalization;
- JSON escaping and appending of the most recent log entries.

The dedicated interface is [include/management-log.h](../include/management-log.h).
The existing callers are `src/main.c`, `lib/syslog/syslog.c`, and the
authenticated management status handler. The ESP-IDF source component already
discovers `src/*.c` through its existing CMake source glob, so no parallel
source list was introduced. The component registration was rechecked by the
build after CMake reconfiguration.

No route, response field, log capacity, timestamp format, or persistence
behavior is intentionally changed.

## Branch and validation rules

- Start each stage from the latest `main` unless a documented dependency is
  required and explicitly recorded.
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

After Stage 1 is reviewed and merged, the recommended next slice is
`management-status.c`. It has a read-only boundary and will make the
factory-reset and UPS-state work easier to trace without moving credentials,
certificate material, or session authorization in the same change.
