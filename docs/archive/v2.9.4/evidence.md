# ESP32-NUT v2.9.4 release evidence

## Scope

This is a security and placement audit release. It makes no PSRAM move because
the measured candidates are both small and hot: `ManagementSession` is 144
bytes; two `portMUX_TYPE` objects are 8 bytes each; and login counters occupy
12 bytes, for about 172 bytes of persistent internal state. Session cookie and
CSRF values are secrets; each is read or changed on authenticated requests.
The login state and locks are accessed for every login/cooldown operation.

API-token records are persisted in NVS and only materialized as bounded,
zeroized operation-local stores; moving them to PSRAM would not reduce the
persistent internal footprint and would weaken the intended secret-memory
boundary. No lock or authorization state moves in this release.

## Acceptance

- Target debug information confirms the 144-byte `ManagementSession` and
  8-byte target lock objects.
- The unchanged session/CSRF, constant-time comparison, zeroization, login
  cooldown, NVS token persistence, and scoped-token contracts were reviewed.
- Exact tag `v2.9.4` resolved to `f2d7c667e`; ESP-IDF v6.0.2 produced the
  1,362,656-byte artifact (SHA-256
  `3606faa44bf360a9d01875b2ec21fa869c99860fffa364a1a1b95c22ca21bc04`).
- Scoped OTA installed the exact image on 3Dprinter. Authenticated ADMIN,
  50-entry logs, NUT `OL`/57-variable poll, and service boundaries passed.
