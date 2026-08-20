# v2.7.6 — replacement UPS reprobe specification

## Goal and release gate

Starting only from published, target-accepted `v2.7.5`, let the read-only NUT
driver recognize a replacement USB HID UPS on the same ESP32 port without a
manual restart or artificial reconnect wait. The prior device's identity,
status, measurements, matcher, and cached physical identity must not appear in
the new device state. A newly attached device becomes current only after its
own full successful poll.

Before code changes, record the clean `main` base, `v2.7.5` tag/release, and a
healthy Agent/NUT baseline for each available supported UPS. `v2.7.6` is
permitted to OTA-update the target for authorized validation from this release
onward; it does not authorize flashing, factory reset, Git publication, or a
UPS-control action.

## Verified variables and boundaries

| State or interface | Required treatment |
| --- | --- |
| USB connection epoch | `src/usb.c` exposes a task-safe, monotonic attachment-generation read. USB callbacks update only USB-owned presence/generation state; they never call dstate. |
| Active device generation | `usbhid-ups` records the generation only after it has initialized the currently opened device. A changed generation invalidates that instance in the driver task before reprobe. |
| Driver-instance resources | `hd`, `udev`, descriptor tree, report buffer, active subdriver, exact matcher, and matcher chain are reset/rebuilt only by the driver task, once per lost/replaced device. |
| Identity and cache | Clear external physical UPS variables and every `driver.cached.ups.*` identity value before probing a replacement. The configured NUT service name remains intact. |
| Freshness | Retain v2.7.2 immediate management invalidation and v2.7.3 monotonic stale/purge behavior. Only the replacement's full successful poll calls `dstate_dataok()`. |
| v2.7.5 behavior | Preserve its bounded parser/claim/reconnect hardening. An unsupported replacement remains graceful stale/unavailable rather than restoring the prior UPS. |
| Security/service limits | Preserve HTTPS `443`, refused `8080`, read-only NUT `3493`, ADMIN/CSRF, bearer scope isolation, Authorization zeroization, NVS, and all UPS-control prohibitions. |

## Luna execution plan

Use `gpt-5.6-luna` with `medium` reasoning for the evidence, edit, and test
loops; raise to `high` only for a reproduced lifetime, matcher, or concurrency
failure. Keep one branch, `feature/ups-change-without-wait`, one reviewable
acceptance boundary, and one serial-monitor owner. Read `AGENTS.md`, current
status, this specification, and files reached by the observed path. Search
before reading inherited driver files; do not preload archives or history.

1. **Capture replacement evidence.** With the fingerprint-pinned diagnostic
   credential retrieved through the 1Password MCP environment, capture Agent
   status, `LIST UPS`/`LIST VAR`, selected subdriver, uptime, and stale state
   for UPS A. Physically disconnect A, attach UPS B, and poll Agent status at
   one-second intervals until B has full-poll data. Repeat B-to-A if both
   physical models are available. Do not write credentials, headers, addresses,
   or tokens to source, shell history, tracked evidence, or chat.
2. **Trace ownership and choose the smallest repair.** Inspect only
   `src/usb.c`, `src/drivers/usbhid-ups.c`, `src/drivers/dstate.*`, and direct
   matcher/parser helpers. Confirm how USB loss/attachment is observed, why
   the old `exact_matcher` prevents replacement selection, and which cached
   identities could be restored after stale expiry. Add the smallest
   USB-owned generation signal and driver-owned reset/reprobe path; do not
   call dstate or free driver resources from a USB callback.
3. **Reprobe safely.** On loss or a changed connection generation, mark data
   stale, clear previous physical state, release/reset the old matcher and
   report resources, restore broad evidenced subdriver matching, then use the
   normal bounded reconnect cadence. On a claim/parse failure, keep stale and
   leave no prior identity. On a successful replacement full poll, cache only
   the replacement identity and make it current. Do not add a new token,
   endpoint, NVS value, device-specific guess, or UPS write.
4. **Build and use authenticated OTA.** Run `git diff --check` and the
   ESP-IDF v6.0.2 `esp32s3` build. Verify the raw application image and
   version locally. Use the existing `ota.install` credential, retrieved only
   through 1Password MCP, to check and install the candidate through the
   authenticated HTTPS route. Confirm rollback validity after reboot; use the
   diagnostic credential only for status/simulation. If OTA is unavailable,
   stop and report rather than falling back to flash without separate authority.
5. **Validate physical replacement.** Repeat A-to-B and B-to-A swaps after
   OTA installation. Confirm there is no artificial wait: B may become current
   only after USB enumeration plus one normal full-poll interval. A physical
   disconnect remains the final acceptance test; the simulation exercises only
   stale presentation and cannot prove replacement behavior.

## Condition and action table

| Condition | Required action |
| --- | --- |
| USB A is healthy | Expose only A's successful full-poll values; record A's active generation. |
| A disappears | Immediately mark stale; browser/Agent show unavailable values; maintain normal NUT `DATA-STALE` behavior. |
| B attaches after A | Driver observes a new generation, clears A's physical dstate/cache and exact matcher, then performs a broad read-only reprobe. |
| B claims and completes a full poll | Cache and expose B values only; clear stale and record B's generation. |
| B enumerates but cannot claim, parse, or fully poll | Remain stale/unavailable with bounded retry/log/resource use; no A values, reboot, watchdog, or control fallback. |
| Diagnostic simulation or actual absence remains active | Keep stale protection; no replacement cache restoration. |
| Reconnect occurs before/after v2.7.3 expiry | Never restore A values into B; B still needs its own full successful poll. |

## Acceptance matrix

| Scenario | Required evidence |
| --- | --- |
| A healthy baseline | Agent/dashboard identity, status, and measurements agree with read-only NUT output; stable uptime and ports. |
| A disconnect | First post-disconnect Agent response and dashboard refresh are stale/unavailable; no cached A values presented as current. |
| A-to-B swap | Before B's full poll, no A identity/value appears in Agent, browser, or NUT variables associated with the new device. After B's full poll, only B values appear. |
| B-to-A swap | Same guarantees in reverse, with no restart, manual driver restart, or artificial wait. |
| Unsupported replacement | Graceful stale/unavailable state, bounded resource/log behavior, no cross-device value, reboot, watchdog, or UPS command. Mark not target-tested if no device exists. |
| v2.7.2/2.7.3 regression | Physical disconnect, simulation, stale timeout/purge, and full-poll recovery preserve their contracts. |
| Authorization and services | Diagnostic versus OTA bearer isolation, ADMIN/CSRF, HTTPS `443`, NUT `3493`, refused `8080`, and OTA rollback validation pass. |

## Release evidence and documentation closeout

Before commit, update the active development plan with the exact v2.7.6
contract/link and current status with branch/base, authorized OTA capability,
observed evidence, gaps, and one next action. Update `NAVIGATION.md` if the
new USB generation/reprobe entry point changes navigation. Update
`ESP32_SECURITY.md`, `ESP32_PREFLIGHT.md`, and roles guidance only if their
authorization, token, certificate, or physical-operation contract changes;
otherwise record that the review found no change needed.

Before publication, create `docs/archive/v2.7.6/evidence.md` with the merged
commit, exact release artifact/checksum, automated validation, target OTA
result, physical A-to-B/B-to-A results, observed/not-tested matrix rows, and
rollback result—never secrets or network coordinates. After the authorized tag
and GitHub release, move this specification into that archive directory,
update `docs/archive/README.md`, replace the active-roadmap entry with the
archived evidence/specification link, and compact `ESP32_CURRENT_STATUS.md`
to the published v2.7.6 baseline and v2.7.7 next action. Inspect all touched
links and run `git diff --check` before handoff.

## Edge cases, stop conditions, and rollback

Test absent-at-boot, fast A-to-B attachment, repeated attach/detach, B
enumeration without a full update, unsupported B, disconnect during report
work, stale expiry during a swap, reboot/OTA while stale, and a USB generation
change while an Agent request is served. A double-free/use-after-free,
reboot/watchdog, stack or heap regression, tight reconnect loop, stale A value
in B state, attempted UPS control, or authorization/service regression stops
the release.

If validation fails, use the authenticated dual-OTA path to target-accepted
v2.7.5. No NVS migration, factory reset, token recreation, or USB recovery is
required; diagnostic and OTA token stores remain unchanged. Rollback
verification repeats healthy status, physical stale protection, read-only
`3493`, HTTPS `443`, and the absence of `8080`.
