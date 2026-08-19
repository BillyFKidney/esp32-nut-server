# v2.7.2 validation evidence index

This index records the evidence for the UPS disconnect-invalidation and Agent
diagnostics release. The release source is merge commit `623cc5f62`; the
annotated `v2.7.2` tag and release artifacts are published only after the
tagged image passes the authorized OTA verification.

| Evidence | Result |
| --- | --- |
| [v2.7.2 specification](../../ESP32_V2_7_2_DISCONNECT_INVALIDATION_SPEC.md) | Scope, invariants, bounded simulation, security conditions, and rollback plan |
| Source review | `git diff --check` passed for tracked and newly added files |
| Build | ESP-IDF v6.0.2 `esp32s3` build passed; `nut-esp32s3.bin` fits the smallest OTA partition |
| Agent checks | Fingerprint-pinned diagnostic and OTA scopes were isolated; malformed requests, revocation, stale simulation, recovery, and NUT `DATA-STALE` behavior passed |
| Physical acceptance | UPS disconnect invalidated cached data immediately; reconnect restored data only after a successful driver update |

The release contains no credentials, tokens, device addresses, or private
certificate material. Release assets are the tagged application image and its
SHA-256 checksum.
