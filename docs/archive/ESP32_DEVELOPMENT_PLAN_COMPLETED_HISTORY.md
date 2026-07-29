# ESP32-NUT completed development history

This archive preserves completed foundation, development-OTA, v1.x, completed v2.x slice, v2.7.0 acceptance, and release-sequencing entries moved out of the active development plan on 2026-07-29. Each moved source block is retained verbatim between its marker; nothing was deleted.

Current roadmap: [ESP32_DEVELOPMENT_PLAN.md](../ESP32_DEVELOPMENT_PLAN.md)
Archive index: [docs/archive/README.md](README.md)

<!-- BEGIN MOVED CONTENT: ESP32_DEVELOPMENT_PLAN.md lines 58-103 -->
## Completed foundation slices and releases

| Milestone | Status | Evidence / outcome |
| --- | --- | --- |
| Adopt the ESP32 alpha port | Complete | Preserved `banoz/nut` ancestry was merged into `main`. |
| ESP-IDF 6.0.2 / ESP32-S3 build and boot | Complete | Build and flash validated on the target board. |
| USB UPS discovery | Complete | USB host detects and describes the connected UPS. |
| CyberPower HID polling | Complete | CyberPower CST150UC2 polling is active and reports read-only state. |
| NUT network service | Complete | Read-only NUT server is reachable on TCP port 3493. |
| Wi-Fi provisioning and recovery | Complete | Open fallback AP/captive portal, saved credentials, and recovery reset path validated. |
| DHCP compatibility | Complete | ESP32-side offered-address probe is disabled after validated UniFi interoperability testing. |
| Project release 1.0.0 | Complete | `v1.0.0` is tagged on `main` and published on GitHub. |
| Development OTA baseline | Complete | `v1.1.0` validated a Wi-Fi upload between both OTA slots, automatic restart, rollback support, NUT access, and CyberPower `ups.status = OL`. |
| Operational Management foundation | Complete | `v2.0.0` publishes the PR #10 LAN-only HTTPS, ADMIN-authentication, initial status/OTA-route, and stack-safe startup foundation. |
| Documentation and workflow continuity patch | Complete | `v2.0.1` publishes the reusable project starter kit, explicit service/workflow-continuity rules, milestone/slice version mapping, and release-gap checks without changing firmware behavior. |
| ADMIN password management | Complete | `v2.1.0` publishes first-run setup, password changes, session expiration, login throttling, authenticated Safari OTA, and target-validated physical ADMIN recovery through PR #12. |

## Development OTA baseline

Release `v1.1.0` included a development-only OTA HTTP server after station
Wi-Fi connected.

- `GET http://<device-ip>:8080/` reports the running and next OTA slots.
- `POST http://<device-ip>:8080/ota` accepts a complete ESP-IDF application
  image, verifies it, selects the inactive slot, and restarts.
- ESP-IDF rollback marks a newly booted image valid after core services start;
  a boot failure before then reverts to the previous image.
- The first full OTA update was validated from `app0` to `app1`, followed by
  Wi-Fi, NUT TCP, and CyberPower `ups.status = OL` checks.

That endpoint was intentionally unauthenticated for trusted-LAN development.
It was retired during the initial Operational Management work before a secure
Agent-driven replacement had been validated, which materially changed the
development workflow. `v2.1.0` restores user-approved Safari OTA, and published
`v2.3.0` restores scoped Agent-driven OTA without ADMIN-password disclosure.
The unauthenticated listener must never be restored as a production
mechanism; any service retirement or replacement now requires the explicit
approval and continuity review defined below.

## Major-version umbrella milestones

### 1. Foundation and development OTA — `v1.x` complete

Released as `v1.1.0`. A clean ESP-IDF v6.0.2 build was installed over Wi-Fi,
alternated OTA slots, restarted automatically, and preserved Wi-Fi, NUT, and
CyberPower UPS monitoring.
<!-- END MOVED CONTENT: ESP32_DEVELOPMENT_PLAN.md lines 58-103 -->

<!-- BEGIN MOVED CONTENT: ESP32_DEVELOPMENT_PLAN.md lines 154-171 -->
#### Implementation slices

Operational Management is an umbrella milestone, not a single long-running
feature branch. Deliver it through small branches that begin at the latest
`main`, have one coherent acceptance boundary, and merge independently after
proportional build and target-hardware validation.

| Order | Release target | Branch | Scope and merge boundary |
| --- | --- | --- | --- |
| 1 | `v2.0.0` | `feature/operational-management` | HTTPS and ADMIN authentication foundation: device certificate, initial password setup, secure browser session, CSRF and login throttling, initial status/OTA routes, and stack-safe startup. Delivered by PR #10 and published as `v2.0.0`. |
| 2 | `v2.1.0` | `feature/admin-password-management` | Complete and validate initial setup, password change, session expiration, login throttling, and physical password recovery. The authenticated Safari OTA picker was pulled forward with explicit approval to restore the development workflow before the retired service's replacement branch. Delivered by PR #12 and published as `v2.1.0`. |
| 3 | `v2.2.0` | `feature/time-configuration` | Establish device-owned time before timestamp-consuming slices: synchronize through configurable NTP with `pool.ntp.org` as the default, provide a manual date/time fallback, store the selected IANA time-zone name with `America/Los_Angeles` as the default, and expose UTC/local time plus synchronization state through the authenticated status API and console. Delivered by PR #16 and published as `v2.2.0`. |
| 4 | `v2.3.0` | `feature/api-tokens` | Complete. PR #20 merged the validated token lifecycle and scoped Agent OTA at `595e3dcda`; annotated tag `v2.3.0` and the final GitHub release are published. |
| 5 | `v2.4.0` | `feature/management-dashboard` | Expose and render the required firmware, Wi-Fi, NUT, UPS, voltage, battery, load, runtime, update, and time diagnostics. |
| 6 | `v2.5.0` | `feature/wifi-management` | Add the client-side ADMIN tab bar and Wi-Fi Configuration panel; scan supported networks, show signal strength, provide an off-by-default local `Show password` toggle, confirm credential changes, reconnect safely, and never reveal the stored password. |
| 7 | `v2.6.0` | `feature/local-ota-management` | Complete. PR #24 merged local Check/Install controls, corrupt-image rejection, release-link guidance, and rollback/persistence validation at `1d2e18acc`; annotated tag `v2.6.0` and the final GitHub release are published. |
| 8 | `v2.7.0` | `feature/live-diagnostics-nut-fields` (first slice); `feature/development-build-identity` (second slice) | Complete and publish the accepted CPU-free hardware diagnostics, bounded runtime logs, server-authoritative ADMIN countdown, explicit activity refresh, and stale-cookie cleanup. Follow-up compatibility and state-invalidation fixes are tracked separately as `v2.7.1` through `v2.7.6`. |
| 9 | `v2.8.0` | `feature/physical-recovery` | Complete and validate the three-second Wi-Fi reset and fifteen-second factory-reset behavior and scope. |
<!-- END MOVED CONTENT: ESP32_DEVELOPMENT_PLAN.md lines 154-171 -->

## v2.7.0 scope and acceptance history

<!-- BEGIN MOVED CONTENT: ESP32_DEVELOPMENT_PLAN.md lines 174-241 -->
The Project Maintainer requested the tabbed ADMIN-console navigation and the
Wi-Fi `Show password` toggle for `v2.5.0` on 2026-07-21. The tab bar is a
presentation-only extension of the existing single-page console, so it does
not add a service, alter authentication boundaries, or require additional
release slices. If implementation risk grows beyond that boundary, split the
tab shell before merging rather than expanding the Wi-Fi-management branch
silently.

The Project Maintainer expanded the `v2.7.0` live-diagnostics scope on
2026-07-22. The slice must add the standard NUT battery chemistry,
battery-manufacturing-date, and UPS-temperature fields when available and
report runtime ESP32 chip, board-profile, memory, and chip-temperature
information. CPU-utilization monitoring was evaluated and removed after target
validation showed unacceptable service impact.

The same slice adds a toolbar warning during the final five minutes of the
existing fifteen-minute idle ADMIN session. The deadline is server
authoritative, background diagnostics must not refresh an idle session, normal
administrator activity may refresh it, and reaching zero reloads the page so
the server presents the sign-in screen. A `401` or `403` from an authenticated
request must also cause the browser to reload to sign-in.

**Observed on 2026-07-22 00:48 PDT:** the first v2.7.0 slice was created as
`feature/live-diagnostics-nut-fields` from `main` at `748d0c77a`. It changes
only `src/management.c`, where the existing NUT dstate snapshot now reads
`battery.type`, `battery.mfr.date`, and `ups.temperature`, serializes them as
`ups.battery_type`, `ups.battery_mfr_date`, and `ups.temperature`, and
renders absent values as `Not available`. The ESP-IDF v6.0.2 build passed at
1,307,696 bytes with SHA-256
`89f21ed093d8dbad4dadc1abdf62f742c50e4643abf7d38f6a031eb71bd651f3`.
Target behavior was validated from the user-provided authenticated status JSON
and Chrome screenshot: `.173` reports healthy read-only NUT with all three
optional values unavailable, and the dashboard renders `Not available`. The
firmware identity still reports `v2.6.0` because the root `version.txt` is
hard-coded to that release; a separate development-build-identity slice must
resolve this provenance gap before the next diagnostics slice. The screenshot
does not independently expose the FQDN address bar.

**Observed on 2026-07-22 01:11 PDT:** the second v2.7.0 slice was created as
`feature/development-build-identity` from the dirty first-slice state at
`748d0c77a`. The root `CMakeLists.txt` now derives `PROJECT_VER` from Git's
tag/commit/dirty description during configure or reconfigure, without changing
tracked `version.txt`. The ESP-IDF v6.0.2 build reports and embeds
`v2.6.0-6-g748d0c77a-dirty`; it passed `git diff --check`, produced a
1,307,696-byte image, and left 61% of the smallest application partition free.
The local image SHA-256 is
`6be41c121a192ab976238d61e899a8e95d581a9392e25ecc9c1c5e5e411f686b`.
No device, release, or GitHub state changed.

The build identity is evaluated at CMake configure time. A branch or dirty
state change therefore requires a reconfigure before the identity changes; a
Git-unavailable build falls back to the unchanged tracked `version.txt` through
ESP-IDF's existing project-version resolution.

**Observed target acceptance on 2026-07-22 01:29 PDT:** authenticated Chrome at
the required FQDN displayed `v2.6.0-6-g748d0c77a-dirty`; the protected Device Status
raw JSON parsed successfully and matched the handoff payload exactly. It
reported the expected ADMIN/HTTPS boundary, `app1`/`app0` OTA slots, healthy
read-only NUT, and unavailable optional UPS fields. Read-only `.173` network
checks succeeded on TCP 443 and 3493, with direct HTTPS HTTP 200. No request
was sent to `.87`, and no serial monitor was opened. The local `upsc` client
was unavailable, so a separate direct NUT client query was not tested.

**Not tested:** clean tagged-build behavior, Git-unavailable fallback, and
identity refresh after a later branch/dirty-state change were not tested in
this slice. During any long target test, the operator should manually refresh
Chrome at least every ten minutes; this is not a background diagnostic
keepalive.
<!-- END MOVED CONTENT: ESP32_DEVELOPMENT_PLAN.md lines 174-241 -->

## v2.x sequencing and publication history

<!-- BEGIN MOVED CONTENT: ESP32_DEVELOPMENT_PLAN.md lines 276-287 -->
The Project Maintainer approved moving time configuration ahead of API tokens
on 2026-07-19 so tokens, dashboards, OTA results, and diagnostics share one
device-owned timestamp model. This deliberately moved restoration of scoped
Agent-driven OTA from `v2.2.0` to `v2.3.0`. The existing authenticated Safari
OTA path remains available, and the required browser-assisted `v2.2.0`
installation was completed before the Agent replacement path was delivered.
No service was retired by this reorder.

These rows identify release targets; completed rows may also identify existing
tags. `v1.0.0`, `v1.1.0`, `v2.0.0`, `v2.0.1`, `v2.1.0`, `v2.2.0`, and `v2.3.0`
are tagged and published. The Project Maintainer explicitly authorized
`v2.3.0` after its acceptance boundary passed.
<!-- END MOVED CONTENT: ESP32_DEVELOPMENT_PLAN.md lines 276-287 -->
