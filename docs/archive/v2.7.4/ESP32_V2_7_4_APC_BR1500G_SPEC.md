# v2.7.4 — APC Back-UPS RS 1500G compatibility specification

## Goal and release gate

Starting only from the published, target-installed `v2.7.3` release, make the
specific APC Back-UPS RS 1500G test unit communicate through the existing
read-only USB HID/NUT path without a freeze, reboot, or presentation of
unvalidated data. This is not a claim of universal APC support.

Before implementation, record a clean `main` base, the published release/tag,
and a healthy CyberPower baseline. The Device Operator prepares a normal
post-factory-reset configuration for the test unit; v2.7.4 does not change
factory-reset behavior, NVS layout, or stored credentials.

## Verified variables and boundaries

| Variable or boundary | Required treatment |
| --- | --- |
| Target UPS | APC Back-UPS RS 1500G only; record USB descriptors and observed subdriver selection without credentials or addresses. |
| USB/HID state | `usb_hid_device_ready()` remains the immediate presence signal; callbacks do not mutate NUT dstate. |
| Driver freshness | `dstate_is_stale()` and a full successful poll retain the v2.7.3 stale/expiry contract. Enumeration alone is not successful communication. |
| Status surfaces | Browser status and bearer Agent status use the same stale boundary; only successful values are shown. |
| NUT service | Preserve read-only TCP `3493` and its existing stale-client behavior. |
| Management | Preserve LAN-only HTTPS `443`, refusal of `8080`, ADMIN/CSRF, bearer scope isolation, and Authorization zeroization. |
| UPS safety | Do not issue UPS commands, change writable mappings, or activate any control path. |

## Luna execution plan

Use `gpt-5.6-luna` for this work. Keep one branch,
`feature/apc-br1500g-support`, and one reviewable acceptance boundary. Read
`AGENTS.md`, current status, this specification, and only the files named in
the active slice. Search before opening a whole driver file; do not preload the
archive or project history.

1. **Reproduce and classify.** With the approved APC physical setup, collect a
   healthy CyberPower baseline, then APC Agent status, NUT `LIST UPS`/read-only
   queries, and bounded driver logs. Poll Agent status at short intervals and
   compare uptime to distinguish a hang, restart, stale state, failed claim,
   and successful communication. Use one serial-monitor owner only if network
   evidence is insufficient.
2. **Identify the mapping path.** Inspect only the observed code path in
   `src/drivers/usbhid-ups.c`, `src/drivers/apc-hid.c`, descriptor/parser code
   reached by that path, and `src/drivers/dstate.c`. Confirm the descriptor,
   claim result, report IDs, and first unsafe/missing value before choosing a
   fix. If the failure cannot be reproduced or isolated, stop after documenting
   evidence; do not add speculative model matching.
3. **Apply the smallest safe repair.** Harden only the proven failed parser,
   mapping, or state transition. Treat absent/invalid/short reports as
   unavailable or stale, never as a fatal process condition or cached success.
   Keep APC writable/control table entries inactive in the ESP32 build.
4. **Validate recovery.** Rebuild with ESP-IDF v6.0.2 for `esp32s3` and run
   `git diff --check`. Confirm CyberPower remains healthy, then repeat the APC
   baseline, sustained polling, disconnect/reconnect, and stale/recovery path.
   Update focused docs only after observed acceptance evidence exists.

## Conditions and acceptance

The Device Operator authorizes each physical connect/disconnect, OTA, flash,
or reset separately. The Agent may use the scoped diagnostics token from the
1Password developer environment for bearer Agent status and simulated stale
checks; it may not expose secrets, substitute simulation for APC hardware, or
perform an OTA without explicit authorization.

Acceptance requires all of the following:

- The APC completes USB enumeration and at least one full successful poll, or
  fails gracefully as stale/unavailable without freeze or reboot.
- During 15 minutes of observed APC operation plus three physical reconnect
  cycles, uptime stays continuous and status/NUT results agree with driver
  freshness.
- Every exposed APC identity, status, and measurement is either observed from
  a successful poll or explicitly unavailable; stale data is never presented
  as current.
- CyberPower regression: healthy poll, disconnect stale protection, and
  recovery after a full poll remain intact.
- HTTPS `443`, NUT `3493`, refused `8080`, browser ADMIN/CSRF behavior, and
  diagnostic-versus-OTA token isolation remain unchanged.

## Observed candidate evidence

- The original APC failure was isolated to the generated CyberPower-only
  `vendorid`/`productid` filters; the driver aborted after enumeration because
  no matching HID UPS remained.
- The candidate removes those filters, keeps the configured NUT service name,
  and was built with ESP-IDF v6.0.2 for `esp32s3`, with `git diff --check`
  passing.
- OTA installation succeeded. The APC completed full polls and remained
  healthy for more than 17 minutes of continuous Agent status observation.
- Three physical disconnect/reconnect cycles passed: immediate stale status,
  no cached UPS data while absent, and recovery after a full poll.
- HTTPS `443`, read-only NUT `3493`, refused `8080`, and OTA-versus-diagnostics
  bearer isolation were verified. CyberPower boot, disconnect stale protection,
  and full-poll recovery passed; browser dashboard presentation is not
  target-tested on this candidate.
- One earlier status-observation window had an unexpected reboot near 13
  minutes. A subsequent observation exceeded 17 minutes without a reboot, so
  the event did not reproduce but remains an unresolved release risk.

## Edge cases and stop conditions

- APC USB enumeration without a full update, repeated failed reports, malformed
  or short descriptors, unsupported product IDs, and reconnect during stale
  expiry must remain bounded and recoverable.
- A reboot, watchdog reset, stack overflow, heap/resource regression, USB task
  deadlock, or any attempted UPS control is a stop condition. Preserve logs and
  rollback rather than widening scope in the same slice.
- Do not infer support from vendor ID, a claim alone, or a single status field.
  Do not claim a physical test passed when the APC is not attached.

## Rollback and handoff

Before release, validate the candidate on the physical APC and CyberPower
matrix. If it fails, use the normal dual-OTA path to return to accepted
`v2.7.3`; no NVS migration, factory reset, or credential change is permitted.
Record branch/base, tested hardware, observed outcome, and the exact next
action in `ESP32_CURRENT_STATUS.md`. Archive this specification and release
evidence only after v2.7.4 publication.
