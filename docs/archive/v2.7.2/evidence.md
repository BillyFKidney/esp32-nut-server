# v2.7.2 validation evidence index

This index records the evidence for the UPS disconnect-invalidation and Agent
diagnostics release. The release source is merge commit `623cc5f62`; the
annotated `v2.7.2` tag names evidence commit `abf517f16`.

| Evidence | Result |
| --- | --- |
| [v2.7.2 specification](../../ESP32_V2_7_2_DISCONNECT_INVALIDATION_SPEC.md) | Scope, invariants, bounded simulation, security conditions, and rollback plan |
| Source review | `git diff --check` passed for tracked and newly added files |
| Build | ESP-IDF v6.0.2 `esp32s3` tagged build passed; `nut-esp32s3.bin` fits the smallest OTA partition with 60% free |
| Agent checks | Fingerprint-pinned diagnostic and OTA scopes were isolated; malformed requests, revocation, stale simulation, recovery, and NUT `DATA-STALE` behavior passed |
| Physical acceptance | UPS disconnect invalidated cached data immediately; reconnect restored data only after a successful driver update |
| Release installation | The checksum-verified tagged image was OTA-installed, reported `v2.7.2`, and was marked valid with healthy read-only NUT data |

The release contains no credentials, tokens, device addresses, or private
certificate material. Release assets are `nut-esp32s3.bin` and
`nut-esp32s3.bin.sha256`; the published checksum is
`2be7cd179c60553217637f1f22eb62350ba8e79956b3bfdafca1fdaad939503e`.
