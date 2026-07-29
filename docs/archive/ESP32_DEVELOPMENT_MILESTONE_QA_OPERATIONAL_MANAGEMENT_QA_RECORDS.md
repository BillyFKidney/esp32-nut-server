# Operational Management QA records

This archive preserves completed v2.7.0 QA records moved out of the active Operational Management requirements and decisions document on 2026-07-29. The moved source content is retained verbatim between the marker below; nothing was deleted.

Current requirements and locked decisions: [ESP32_DEVELOPMENT_MILESTONE_QA_OPERATIONAL_MANAGEMENT.md](../ESP32_DEVELOPMENT_MILESTONE_QA_OPERATIONAL_MANAGEMENT.md)
Archive index: [docs/archive/README.md](README.md)

<!-- BEGIN MOVED CONTENT: ESP32_DEVELOPMENT_MILESTONE_QA_OPERATIONAL_MANAGEMENT.md lines 249-319 -->
### v2.7.0 slice 1 QA record: read-only NUT fields

**Observed on 2026-07-22 00:48 PDT:**
`feature/live-diagnostics-nut-fields` adds the existing-driver NUT values
`battery.type`, `battery.mfr.date`, and `ups.temperature` to the protected
status snapshot as `ups.battery_type`, `ups.battery_mfr_date`, and
`ups.temperature`. The ADMIN dashboard renders the three values in a
dedicated UPS-details card and maps missing/unavailable values to exactly
`Not available`. The source change does not add a route, NUT control, flash or
NVS write, or authorization boundary.

The ESP-IDF v6.0.2 build passed for the ESP32-S3 target. The local candidate is
1,307,696 bytes with SHA-256
`89f21ed093d8dbad4dadc1abdf62f742c50e4643abf7d38f6a031eb71bd651f3`, and
`git diff --check` passed.

**Observed from the user-provided authenticated status JSON and Chrome
screenshot on 2026-07-22:** `.173` reports NUT health `ok`, read-only
`ups.status = OL`, and `unavailable` for all three optional fields. The
dashboard renders the UPS-details card with `Not available` for battery type,
battery manufacture date, and UPS temperature. No page-level horizontal
overflow is visible in the supplied screenshot; `.87` remains untouched.

**Observed provenance gap:** the displayed firmware identity remains `v2.6.0`
because the root `version.txt` is hard-coded to that value. A separate
development-build-identity slice is required to make branch/dirty state
visible in development images without changing release provenance.

**Not tested:** the supplied screenshot does not show the FQDN address bar, so
the browser hostname cannot be independently confirmed from the image alone.
Safari was not used for this validation.

### v2.7.0 slice 2 QA record: development build identity

**Observed on 2026-07-22 01:11 PDT:**
`feature/development-build-identity` adds a configure-time Git identity in the
root `CMakeLists.txt`. When Git metadata is available, `PROJECT_VER` is set from
`git describe --tags --dirty --always`; the tracked `version.txt` remains
unchanged for release provenance and fallback behavior. With the current dirty
worktree, the ESP-IDF v6.0.2 build reported and embedded
`v2.6.0-6-g748d0c77a-dirty`.

**Passed locally:** ESP32-S3 build completed; `git diff --check` passed; the
image is 1,307,696 bytes, 61% of the smallest application partition remains
free, and the exact local artifact SHA-256 is
`6be41c121a192ab976238d61e899a8e95d581a9392e25ecc9c1c5e5e411f686b`.
No route, authentication, CSRF, HTTPS 443, read-only NUT 3493, retired 8080,
flash/NVS, or device behavior was changed by this slice.

**Passed target acceptance on 2026-07-22 01:29 PDT:** authenticated Chrome at
the required FQDN displayed the firmware-card identity
`v2.6.0-6-g748d0c77a-dirty`. The protected Device Status view exposed raw JSON
that parsed successfully and matched the handoff payload exactly, including
the `ADMIN` role, HTTPS transport, `app1`/`app0` OTA slots, healthy read-only
NUT, and the three unavailable optional UPS fields. Read-only `.173` network
checks succeeded on TCP 443 and 3493, with direct HTTPS HTTP 200. No request
was sent to `.87`; no serial monitor was opened. The local `upsc` client was
unavailable, so a separate direct NUT client query was not tested.

**Not tested:** clean tagged-build behavior, Git-unavailable fallback, and a
later branch/dirty-state reconfigure were not tested.

**Operator acceptance procedure:** upload the exact local candidate through
Chrome at `https://esp32nut-3dprinter.28670avenidacondesa.com/`, then verify the
firmware card and authenticated status JSON report
`v2.6.0-6-g748d0c77a-dirty`. Use direct `.173` only for read-only NUT checks.
During a long test, manually refresh the ADMIN console at least every ten
minutes. If Chrome has already timed out, delete its stale FQDN session cookie
before signing in again; this workaround must not be implemented as a
background diagnostic keepalive.

<!-- END MOVED CONTENT: ESP32_DEVELOPMENT_MILESTONE_QA_OPERATIONAL_MANAGEMENT.md lines 249-319 -->
