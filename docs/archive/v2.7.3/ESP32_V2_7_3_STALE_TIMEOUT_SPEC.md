# v2.7.3 NUT stale-timeout and external-data purge

## Goal and release gate

After a UPS has been stale for five monotonic minutes, purge its external
identity, status, and measurements from dstate once. v2.7.2's immediate stale
presentation remains unchanged: management and NUT clients must never treat
the values as current during the diagnostic window or after expiry.

Begin `feature/nut-stale-timeout` only after confirming the published,
target-accepted `v2.7.2` tag/release and its observed immediate stale
invalidation in status JSON and the dashboard. This is one active contract;
archive it with v2.7.3 release evidence after publication.

## Verified variables and interfaces

| State or interface | Contract |
| --- | --- |
| `dstate_is_stale()` | Starts timing only on a fresh-to-stale transition; repeated failed polls do not extend the deadline. |
| Monotonic `stale_since` | Sole five-minute deadline source; NTP/manual clock changes cannot affect it. |
| Full successful UPS poll | Only event that clears stale timing and permits physical UPS values to become current; USB enumeration alone does not qualify. |
| `dstate_dataok()` / `dstate_datastale()` | Preserve NUT `DATAOK`/`DATA-STALE` behavior while adding one-time expiry/purge ownership to dstate. |
| `nut.available`, `nut.data_stale`, `nut.health` | Remain consistent with v2.7.2 before and after expiry. |
| External UPS values | Purge identity, status, battery, power, input/output, and measurements after expiry; retain internal `driver.*`, configuration, and reconnect machinery. |
| `nut.ups_name` | Preserve configured service identity only; it is not cached physical UPS identity. |

`src/drivers/usbhid-ups.c` supplies poll cadence and the full-update signal.
`src/drivers/dstate.c`/`.h` own timing, one-time purge, and transitions.
`src/server/upsd.c` and `src/server/netlist.c` retain existing stale-client
handling on read-only `3493`; `src/management-status.c` and the dashboard
continue v2.7.2's immediate not-current presentation.

## Agent diagnostic capability

Use the existing `diagnostics.nut` bearer scope only for Agent status and the
bounded disconnect simulation; it never authorizes OTA. `ota.install` remains
the separate scope used only for image upload. Retrieve these credentials and
the certificate-trust material only through the 1Password MCP developer
environment. Do not copy a secret into source, tracked files, shell history,
command output, logs, or chat.

v2.7.3 raises only the bounded diagnostic duration ceiling to 310 seconds,
providing a 10-second observation margin over the default two-second driver
poll interval. It adds no scope, route, USB, NVS, or UPS-control capability.

## Conditions and actions

| Condition | Required action |
| --- | --- |
| Fresh becomes stale | Record monotonic `stale_since` once; preserve internal values only for the diagnostic window. |
| Stale for less than five minutes | Keep v2.7.2's unavailable management presentation and NUT `DATA-STALE`; do not purge or extend timing. |
| Deadline reached | From normal driver polling, purge external dstate values once within one driver poll interval and emit one bounded lifecycle log. |
| USB reappears without full update | Keep the timer and stale state; do not repopulate data. |
| Full successful update | Restore cached static identity, repopulate values, clear stale state and timing/expiry state, then allow normal current presentation. |

Do not change HTTPS `443`, retired `8080`, read-only NUT `3493`,
ADMIN/CSRF/bearer rules, NVS, factory reset, or the UPS-control prohibition.

## Acceptance and manual matrix

Build with ESP-IDF v6.0.2 for `esp32s3`; run `git diff --check`. Before a
physical test, use the fingerprint-pinned 1Password-backed diagnostic token to
verify status, wrong-scope rejection, malformed simulation rejection, and
bounded simulated stale/recovery behavior. Use the OTA token only for the
separately authorized image upload.

| Scenario | Required evidence |
| --- | --- |
| Healthy baseline, then disconnect | Immediate unavailable dashboard/status values and NUT `DATA-STALE`; record stale start. |
| Five-minute expiry | At deadline plus at most one poll interval, external dstate values are purged once and lifecycle log is bounded. |
| Reconnect before / after expiry | A full poll restores current data in both cases; enumeration alone does not. |
| Repeated failed polls | Deadline is not extended and purge happens once. |
| Browser and Agent status | Both retain v2.7.2 not-current behavior; diagnostic bearer does not refresh browser activity. |
| Restart/OTA while stale; absent boot | No crash, no retained current data, and normal recovery after a full poll. |
| NTP/manual clock change | No effect on the monotonic deadline. |
| Ports and sessions | HTTPS `443`, NUT `3493`, refused `8080`, and existing ADMIN/CSRF behavior remain unchanged. |

The final acceptance remains physical: disconnect the UPS with no simulation
active, observe immediate stale invalidation and timed purge, reconnect it,
and verify recovery only after a full successful update.

## Edge cases and rollback

Expiry must not depend on browser refresh, an Agent request, or successful USB
enumeration. The normal driver poll path owns dstate mutation; no parallel task
may walk or purge its state tree. Expiry must not repeat the lifecycle log, mask
a real continuing absence, or carry across a full successful update incorrectly.
v2.7.4-v2.7.7 must not assume stale identity or measurements survive the expiry.

If v2.7.3 fails validation, use the normal dual-OTA path to restore accepted
v2.7.2. No NVS migration, factory reset, credential recreation, or physical
recovery is required. Rollback verification repeats HTTPS `443`, NUT `3493`,
stale protection, and healthy full-poll recovery.
