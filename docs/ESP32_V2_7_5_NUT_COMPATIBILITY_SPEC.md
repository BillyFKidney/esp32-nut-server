# v2.7.5 — NUT compatibility-hardening specification

## Goal and release gate

Starting only from the published, target-accepted `v2.7.4` release, make the
read-only USB HID/NUT path fail safely and predictably for the two evidenced
devices (CyberPower CST150UC2 and APC Back-UPS RS 1500G), and for an
unsupported or malformed USB HID UPS. This release does not claim universal
UPS support and does not implement immediate replacement-UPS reprobe; that is
the separate `v2.7.6` acceptance boundary.

Before implementation, record the clean `main` base, `v2.7.4` tag/release,
and healthy Agent-status baselines for both supported devices. Do not begin
implementation from an uncommitted v2.7.4 candidate or treat enumeration as a
successful UPS poll.

## Verified variables and boundaries

| State or interface | v2.7.5 contract |
| --- | --- |
| HID presence | `usb_hid_device_ready()` remains presence-only. No USB callback may mutate dstate or present data as current. |
| Subdriver selection | The compiled `usbhid-ups` list is limited to evidenced, read-only subdrivers. A claim failure is a bounded unavailable/stale outcome, never a crash or control fallback. |
| Report descriptor and poll | Descriptor parsing, report walking, and missing/short/invalid values are bounded. A full successful poll alone calls `dstate_dataok()`. |
| Freshness and purge | Retain the v2.7.2 immediate stale-invalidation and v2.7.3 five-minute monotonic external-data purge contracts unchanged. |
| Device identity | Only values from the currently successful full poll may be exposed as current. The configured NUT service name remains an internal service identity. |
| Reconnect policy | Keep v2.7.4's existing-device matcher and normal reconnect cadence. Do not make a newly attached UPS replace a prior one without the v2.7.6 design and acceptance. |
| Services and authority | Preserve HTTPS `443`, refused `8080`, read-only NUT `3493`, ADMIN/CSRF, bearer scope isolation, and Authorization zeroization. |

## Luna execution plan

Use `gpt-5.6-luna` with `medium` reasoning for the bounded evidence and
implementation loops below; use `high` only to investigate a reproduced
driver failure. Keep one branch, `feature/nut-compatibility-hardening`, and
one reviewable acceptance boundary. Read `AGENTS.md`, current status, this
file, and only the files reached by the observed path. Search before opening a
whole inherited driver file; do not preload the archive or project history.

1. **Establish the v2.7.4 baseline.** Use the fingerprint-pinned,
   1Password-MCP-provided diagnostic credential only for Agent status and the
   bounded disconnect simulation. Record a healthy full-poll result for each
   available supported UPS, the selected subdriver, steady uptime, NUT
   read-only queries, and stale/recovery behavior. Use the OTA credential only
   after a separately authorized build installation.
2. **Classify a failure before changing code.** For each observed unsupported,
   malformed, or failing device, determine whether failure is at USB opening,
   exact matcher, subdriver claim, descriptor parsing, report allocation,
   report walk, or reconnect. Inspect only `src/usb.c`,
   `src/drivers/usbhid-ups.c`, the selected subdriver, `src/drivers/dstate.*`,
   and their directly called parser/allocation helpers. Preserve bounded logs
   and uptime evidence; do not add a vendor/product allowlist by guesswork.
3. **Apply the narrowest proven hardening.** Validate lengths/counts before
   parsing or allocating from a device report; release partial resources on a
   failed claim/parse; and route a recoverable error through normal stale and
   reconnect handling. Leave all writable mappings and UPS command paths
   inactive. Do not broaden the compiled subdriver set or merge v2.7.6's
   replacement-device behavior into this release without physical evidence.
4. **Exercise bounded failure and recovery.** Rebuild with ESP-IDF v6.0.2 for
   `esp32s3` and run `git diff --check`. Repeat healthy polling and physical
   disconnect/reconnect for both available supported devices. If an
   unsupported test device is available, attach it only with Operator
   authorization and prove it remains stale/unavailable without watchdog,
   reboot, heap trend, busy reconnect loop, or UPS write. Update evidence only
   after these observations.

## Conditions and required actions

| Condition | Required action |
| --- | --- |
| USB HID device claims an evidenced subdriver and completes a full poll | Publish only successful values, call `dstate_dataok()`, and preserve ordinary read-only NUT service. |
| USB enumerates but no subdriver claims it | Keep data stale/unavailable, use bounded retry/log behavior, and retain service availability without a crash or restart. |
| Descriptor is missing, oversized, malformed, or cannot be parsed | Reject it before unsafe traversal/allocation; clean up and follow normal stale/reconnect flow. |
| A report is short, invalid, or a full poll fails | Do not copy/cache a value as success; retain stale protections and let v2.7.3 timing continue. |
| Repeated reconnect or claim failure | Respect existing poll/reconnect pacing; no tight loop, unbounded log growth, socket leak, heap decline, or task blockage. |
| Supported UPS disconnects/reconnects | Immediately expose stale/unavailable data, then recover only after a full successful poll. |
| Another UPS is attached after a prior UPS | Preserve the prior exact-match/reconnect policy and stale safety. Do not advertise immediate replacement support until v2.7.6. |

## Acceptance matrix

| Scenario | Required evidence |
| --- | --- |
| CyberPower CST150UC2 | Healthy full poll, read-only `3493` queries, Agent/dashboard values, physical disconnect stale state, and recovery after a full poll. |
| APC Back-UPS RS 1500G | Same healthy/stale/recovery matrix; stable uptime through the sustained observation defined by v2.7.4. |
| Unsupported or malformed HID UPS | Graceful stale/unavailable state, bounded logs/resources/retries, no reboot/watchdog, and no asserted support. If no such device is available, mark this row not target-tested. |
| Agent isolation | Diagnostic bearer reads/simulates only; OTA bearer cannot use diagnostics; diagnostic bearer cannot upload; revoked/wrong/malformed bearer is rejected. |
| Browser and NUT boundaries | Browser status remains session-only without background activity refresh; `GET`/`LIST` work as applicable on `3493`; `SET`/UPS control remain unavailable. |
| Platform regression | HTTPS `443`, refused `8080`, ADMIN/CSRF, OTA validation/rollback, certificate pinning, and status freshness/purge behavior remain unchanged. |

## Edge cases and stop conditions

- Test absent-at-boot, repeated failed polls, a claim failure after USB
  enumeration, disconnect during descriptor/report work, reconnect before and
  after the v2.7.3 purge deadline, and diagnostic simulation while physical
  USB is absent.
- A reboot, watchdog reset, stack overflow, resource regression, unbounded
  reconnect/log loop, cross-device cached value, attempted UPS control, or
  authorization boundary regression is a stop condition. Preserve evidence and
  roll back rather than expanding scope.
- Do not use a simulated disconnect as evidence of unsupported-device handling,
  physical compatibility, or hot replacement. It tests only the established
  stale boundary.

## Rollback and handoff

Validate the candidate on the available physical matrix before publication.
If v2.7.5 fails, return through the normal dual-OTA path to target-accepted
`v2.7.4`; no NVS migration, factory reset, credential recreation, or USB
recovery is required. Existing OTA and diagnostic token storage and scope stay
unchanged. Record tested hardware, commit, branch, observed/not-tested rows,
and one exact next action in `ESP32_CURRENT_STATUS.md`; archive this
specification with release evidence only after v2.7.5 publication.
