# ESP32-NUT current development status

This is the fast-start handoff for the active branch. Read it immediately after
[AGENTS.md](../AGENTS.md). It intentionally contains current facts, the active
acceptance boundary, and one next action—not release history or reusable
procedures.

Do not preload `docs/archive/` during a normal startup. Historical evidence from
the former 101 KB status file is preserved in
[ESP32_CURRENT_STATUS_HISTORY.md](archive/ESP32_CURRENT_STATUS_HISTORY.md).

## Repository snapshot

**Repository state was rechecked at the start of the management modularization
work; hardware and LAN state were not checked.**

| Field | Current fact |
| --- | --- |
| Active branch | `feature/management-certificates-module` |
| Release target | `v2.7.1` |
| HEAD | Resolve from live Git. This file intentionally does not hard-code its own containing commit |
| Branch base | Temporarily stacked on the local `feature/management-status-module` commit, which is stacked on the local logging commit; rebase the reviewed slices onto merged `main` before publishing; live Git is authoritative |
| Remote branch | No upstream is configured for the active feature branch |
| Implementation state | Logging and read-only status extractions are locally committed; HTTPS certificate/key lifecycle extraction is implemented on this stacked feature branch and target build passed; no factory-reset behavior has been changed |
| Worktree scope | `management-certificates.c`/`management-certificates.h`, explicit component registration, HTTPS-server caller updates, and modular-refactoring documentation are the active scope |
| Published baseline | `v2.7.0`; resolve post-release documentation history from live Git rather than maintaining a count here |
| Target | YD-ESP32-23, ESP32-S3-WROOM-1-N16R8, 16 MB flash, 8 MB octal PSRAM |
| SDK | ESP-IDF v6.0.2, target `esp32s3` |
| Required services | LAN-only HTTPS `443`; read-only NUT `3493`; retired unauthenticated `8080` remains refused |
| Device coordinates | Not checked. Rediscover the IP address and `/dev/cu.usbmodem*` path before hardware work; historical values are not current facts |
| Authorization | No flash, OTA, factory reset, push, merge, tag, release, or other external action is authorized by this handoff |

## Active slice: HTTPS certificate material

The active branch moves the self-signed HTTPS certificate/private-key lifecycle
from `src/management.c` into `management-certificates.c`. The module preserves
the `management` NVS namespace and the `https-cert`/`https-key` blob keys,
stored-pair reuse, missing/incomplete-pair regeneration, and private-key
zeroization. `management.c` remains responsible for HTTPS-server startup,
routes, ADMIN/session/CSRF boundaries, and factory-reset orchestration.

This must not change HTTPS `443`, NUT `3493`, refused `8080`, ADMIN/CSRF,
token, Wi-Fi, or factory-reset behavior.

The staged extraction sequence and later management/Wi-Fi boundaries are in
[ESP32_REFACTORING_PLAN.md](ESP32_REFACTORING_PLAN.md).

This is temporarily a stacked local branch based on the committed logging and
status extractions. Review and merge each preceding slice, then rebase this
branch onto the merged `main` before publishing it.

## Factory-reset slice remains separate

### Reported defect

**Observed by the Project Maintainer after v2.7.0 acceptance:** with the UPS
disconnected, holding BOOT for at least fifteen seconds and completing a
factory reset can leave the previous UPS identity and measurements visible.
It is **not yet proven** whether those values come from persistent storage, a
runtime cache, or another NUT state path.

### Required outcome

`v2.7.1` must make the fifteen-second factory reset erase every value within
the agreed reset boundary, including UPS identity/cache state, while retaining
the current bootable firmware and OTA recovery slot. On restart, stale UPS data
must not be presented as current.

### Initial source facts

- `src/wifi.c` erases the `wifi-config` NVS namespace at the three-second
  threshold and calls `management_factory_reset()` after the fifteen-second
  threshold is reached and BOOT is released.
- `management_factory_reset()` erases the `management` NVS namespace and clears
  the active ADMIN session. That namespace is also used by ADMIN credentials,
  API tokens, device identity/certificate material, time configuration, and OTA
  result metadata.
- The existing reset path does not explicitly invalidate the NUT dstate values
  or other UPS runtime caches before restart.
- A first source inventory found the downstream `wifi-config` and `management`
  NVS namespaces. This does not prove that all retained UPS state is NVS-backed;
  inherited NUT runtime or filesystem paths still require tracing.

### Acceptance boundary

- Reproduce the disconnected-UPS reset case before claiming a fix.
- Define the complete persistent and runtime reset inventory in code or tests.
- Erase the agreed Wi-Fi, ADMIN, token, device, time, OTA-result, optional-log,
  and UPS identity/cache state.
- Retain the current bootable firmware and OTA recovery slot.
- Restart into the documented provisioning/recovery flow.
- Never show pre-reset UPS identity or measurements as current after restart.
- Preserve LAN-only HTTPS `443`, read-only NUT `3493`, refused `8080`, and all
  existing ADMIN/CSRF boundaries.
- Keep UPS access read-only; do not begin APC-specific compatibility work in
  this slice.
- Build with ESP-IDF v6.0.2 and perform proportionate target validation before
  calling the slice complete.

## Exact next action

Review the focused local certificate-extraction commit on
`feature/management-certificates-module`. The target build with ESP-IDF v6.0.2
passed after explicit registration of `management-log.c`,
`management-status.c`, and `management-certificates.c`; no host test harness is
configured for this component. Do not publish this stacked branch before the
preceding slices are reviewed, merged, and rebased onto `main`. After the
modular slices are reviewed/merged, resume the separate factory-reset
investigation by tracing every UPS field exposed by the authenticated status
response back through NUT dstate, runtime caches, and filesystem persistence.

## Read only when needed

| Need | Document |
| --- | --- |
| Active and future release slices | [ESP32_DEVELOPMENT_PLAN.md](ESP32_DEVELOPMENT_PLAN.md) |
| Locked factory-reset and security decisions | [ESP32_DEVELOPMENT_MILESTONE_QA_OPERATIONAL_MANAGEMENT.md](ESP32_DEVELOPMENT_MILESTONE_QA_OPERATIONAL_MANAGEMENT.md) |
| Hardware, network, COM, flash, or OTA work | [ESP32_PREFLIGHT.md](ESP32_PREFLIGHT.md) |
| Authority for physical, external, or destructive actions | [ESP32_DEVELOPMENT_ROLES.md](ESP32_DEVELOPMENT_ROLES.md) |
| Security boundaries | [ESP32_SECURITY.md](ESP32_SECURITY.md) |
| Synology/AdGuard browser path | [ESP32_MANAGEMENT_PROXY.md](ESP32_MANAGEMENT_PROXY.md) |
| Repository file ownership or moves | [ESP32_REPOSITORY_LAYOUT.md](ESP32_REPOSITORY_LAYOUT.md) |
| Completed releases and old device evidence | [Archive index](archive/README.md) |

Update this file only when the branch, base, worktree, implementation state,
validation evidence, authorization state, or exact next action changes. Keep
procedures in preflight, roadmap detail in the development plan, decisions in
the milestone/security documents, and historical evidence in the archive.
Never record passwords, Wi-Fi credentials, cookies, API tokens, private keys,
or Authorization headers here.
