# ESP32-NUT current development status

This is the fast-start handoff for the active branch. Read it immediately after
[AGENTS.md](../AGENTS.md). It intentionally contains current facts, the active
acceptance boundary, and one next action—not release history or reusable
procedures.

Do not preload `docs/archive/` during a normal startup. Historical evidence from
the former 101 KB status file is preserved in
[ESP32_CURRENT_STATUS_HISTORY.md](archive/ESP32_CURRENT_STATUS_HISTORY.md).

## Repository snapshot

**Repository state and development-target smoke evidence were rechecked on
2026-07-29. This file records only current handoff facts; the complete target
evidence is preserved in [2026-07-29 target candidate smoke validation](archive/2026-07-29-target-candidate-smoke-validation.md).**

| Field | Current fact |
| --- | --- |
| Active branch | `feature/ota-check-image-identity` |
| Release target | `v2.7.1` |
| HEAD | Resolve from live Git. This file intentionally does not hard-code its own containing commit |
| Branch base | Stacked on the local `candidate/wifi-provisioning-web-header-limit` integration candidate, which combines the Wi-Fi provisioning-web extraction and the request-header-limit fix; that candidate remains stacked on the local route-inventory, Wi-Fi diagnostic, Wi-Fi credential, HTTP-helper, session, credential, certificate, status, and logging slices. Rebase reviewed slices onto merged `main` before publishing; live Git is authoritative |
| Remote branch | No upstream is configured for the active feature branch |
| Implementation state | Management logging, read-only status, HTTPS certificate/key lifecycle, ADMIN credential, ADMIN session/CSRF, shared HTTP helper, Wi-Fi credential, Wi-Fi diagnostic, route-inventory, and temporary Wi-Fi provisioning-web slices are locally committed and target builds passed. The combined candidate was installed through the authenticated browser OTA path and passed reboot/ADMIN-session smoke checks. The active slice reports the embedded version of a checked local image without changing its validation, installation, or reboot behavior |
| Worktree scope | `src/ota.c`, the Update Firmware panel wording, and concise current/archive documentation. The check response adds a bounded `firmware_version` field and includes the same version in its message only after the existing complete-image validation succeeds |
| Published baseline | `v2.7.0`; resolve post-release documentation history from live Git rather than maintaining a count here |
| Target | YD-ESP32-23, ESP32-S3-WROOM-1-N16R8, 16 MB flash, 8 MB octal PSRAM |
| SDK | ESP-IDF v6.0.2, target `esp32s3` |
| Required services | LAN-only HTTPS `443`; read-only NUT `3493`; retired unauthenticated `8080` remains refused |
| Device coordinates | **Observed:** the development-console FQDN was used for browser OTA and current DNS resolved it to `192.168.40.10`. Treat that as the management-proxy address; rediscover the direct ESP32 address and any `/dev/cu.usbmodem*` path before direct hardware work |
| Authorization | The Project Maintainer authorized the completed browser OTA smoke validation of the integration candidate. No additional flash, OTA, factory reset, push, merge, tag, or release is authorized by this handoff |

## Active slice: checked-image identity

`src/ota.c` already writes a complete checked image to the inactive OTA
partition and calls `esp_ota_end()` before returning the successful check
response. This slice reads the ESP-IDF application descriptor from that same
verified partition and reports its bounded, JSON-safe embedded version in the
ADMIN-and-CSRF-protected response. It does not select the image for boot, alter
the inactive-slot choice, persist a new result, or schedule a restart.

The user-facing check text will identify the checked image. The dashboard after
a successful reboot remains the authoritative source for the version currently
running on the device.

## Stacked prerequisite: Wi-Fi provisioning web module

The active branch moves the temporary captive-portal HTTP handlers, common
portal response helpers, bounded form decoding, JSON construction, and route
registration from `src/wifi.c` into `src/wifi-provisioning-web.c`. It preserves
the four existing portal endpoints and lets `wifi.c` retain Wi-Fi radio state,
credential lifecycle, AP/DNS lifecycle, physical recovery, portal scheduling,
and HTTP server-handle ownership.

This must not change HTTPS `443`, NUT `3493`, refused `8080`, ADMIN/CSRF,
token, certificate, credential validation/staging, Wi-Fi connection/recovery,
or factory-reset behavior. The focused module receives only the existing
connection-request flag and restart callback; it does not make connection or
reset decisions.

The staged extraction sequence and later management/Wi-Fi boundaries are in
[ESP32_REFACTORING_PLAN.md](ESP32_REFACTORING_PLAN.md).

This is temporarily a stacked local branch based on the route-inventory,
logging, status, certificate, credential, session, HTTP-helper, Wi-Fi
credential, and Wi-Fi diagnostic slices. Review and merge each preceding slice,
then rebase this branch onto the merged `main` before publishing it.

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

Build and review `feature/ota-check-image-identity`. With a separate explicit
OTA authorization, use the configured development board to check a local
candidate and confirm that the response names the image's embedded version
before any install is chosen. Temporary-portal target acceptance remains
separate and requires a non-production device or an explicitly approved
recovery session. Do not publish this stacked branch before the preceding
slices are reviewed, merged, and rebased onto `main`. The separate factory-
reset investigation still requires tracing every UPS field exposed by the
authenticated status response back through NUT dstate, runtime caches, and
filesystem persistence.

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
