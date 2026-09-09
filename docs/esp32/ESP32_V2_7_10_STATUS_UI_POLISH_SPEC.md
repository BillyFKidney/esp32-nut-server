# v2.7.10 status UI polish and full-log retrieval

## Goal and release gate

Starting from published, target-accepted `v2.7.9`, improve the ADMIN Device
Status experience while preserving raw status data and the existing security,
service, NVS, OTA, and UPS-control boundaries. This slice intentionally adds
one read-only ADMIN endpoint because the status response contains only its
bounded recent-log window.

The release is ready for implementation only when every requirement and
acceptance case below is traceable. A browser rendering failure, changed raw
status value, unexpected bearer access, session-activity refresh, new service
exposure, stale-data leak, or unbounded response allocation stops the release.

## Scope

1. Map the raw UPS manufacturer value `CPS` to the visible label
   `CyberPower Systems` on the ADMIN page only.
2. Move `Device display name`, `Application Log Level`, and `Save device
   settings` from Dashboard to Device Status, directly below the page title
   and above the raw status text.
3. Keep raw status JSON visible on Device Status without a toggle.
4. Add `Copy JSON` beside the Device Status title. It copies the exact response
   text received by the most recent successful status request.
5. Add `Copy Logs` beside `Copy JSON`. On an explicit click, it retrieves and
   copies every log currently retained by the in-memory management log ring.
6. Add `GET /api/v1/admin/logs`, an ADMIN-session-only read endpoint for that
   full retained log window.
7. Add a repeatable static validation seam for the embedded ADMIN-page
   JavaScript before target validation. On macOS, `idf.py build` must run the
   JXA validator; on other hosts it remains an explicit documented check.

## Explicitly out of scope

- No change to the `/api/v1/status` or `/api/v1/agent/status` schema, log
  window, diagnostic bearer scope, OTA bearer scope, NVS persistence, or NUT
  identity.
- No persistent log store, log export to a remote system, token scope, UPS
  control, Wi-Fi, certificate, factory-reset, port, or reverse-proxy change.
- No runtime memory, HTTPS task, log-capture, or CPU-affinity optimization.
  Those are measured `v2.8.0` candidates.

## Display mapping contract

The mapping is presentation-only and is applied only where the ADMIN page
displays a UPS manufacturer. Raw JSON, NUT service identity, and stored data
are never changed.

| Raw manufacturer | Visible label | Notes |
| --- | --- | --- |
| `CPS` | `CyberPower Systems` | Exact case-sensitive fallback mapping. |
| Any other non-empty available string | Same raw value | Includes other CyberPower strings and APC values. |
| Empty or `unavailable` | Existing `Not available` rendering | No vendor inference. |
| Stale/unavailable UPS state | Existing stale/unavailable rendering | Never render cached identity as current. |

The model, serial number, and all raw JSON fields continue to use their
existing values and freshness rules.

## Status and clipboard contract

`loadStatus()` retains one canonical pair after a successful response:

- `lastStatusText`: the exact unmodified response text.
- `lastStatus`: the parsed object used for rendering.

The page performs one status request per manual or scheduled refresh. It must
not reserialize `lastStatus` for `Copy JSON`, issue a second request merely to
copy JSON, or mutate raw values for friendly presentation.

`Copy JSON` copies `lastStatusText`. Before the first successful status load,
the button is disabled with an explanatory status message.

`Copy Logs` performs one `GET /api/v1/admin/logs` only after the user clicks.
It creates a newline-delimited transcript in the endpoint's oldest-to-newest
order. Each line contains the local timestamp when available, otherwise the
uptime value, followed by the level and message. The page reports the copied
retained-entry count. An empty response copies no data and reports that no
application logs are retained.

Use `navigator.clipboard.writeText()` for both buttons. If permission is
denied or the API is unavailable, expose the prepared text in a visible,
selectable fallback control and give it focus. Clipboard operations remain
user-initiated. Responses and fallback text must never include credentials,
tokens, cookies, private keys, or Authorization headers.

## Full-log endpoint contract

### Route and authorization

| Property | Requirement |
| --- | --- |
| Method and URI | `GET /api/v1/admin/logs` |
| Authorization | Valid ADMIN session cookie only, checked without activity refresh. |
| CSRF | Not required because the route is read-only. |
| Bearer tokens | Rejected; neither `ota.install` nor `diagnostics.nut` gains access. |
| Unauthorized result | Existing `401` ADMIN-session response. |
| Content type | `application/json`. |
| Persistence | None. The endpoint exposes only the current volatile management log ring. |

### Response

The endpoint returns all currently retained entries, ordered oldest to newest.
It does not claim to provide logs overwritten before the current ring window.

```json
{
  "logs": [
    {
      "uptime_ms": 0,
      "timestamp_utc": null,
      "timestamp_local": null,
      "level": "info",
      "message": "example"
    }
  ],
  "returned_count": 1,
  "retained_capacity": 24,
  "status_window": 6,
  "complete_retained_window": true
}
```

- Each entry has exactly the existing status-log field names and escaping
  behavior.
- `retained_capacity` and `status_window` are factual runtime bounds, not
  promises of durable history.
- `complete_retained_window` means every entry still present in the volatile
  ring was returned. It does not claim that older overwritten entries exist.
- HTTPS handshake noise remains excluded under the existing log-capture rule.

### Resource and implementation boundary

The current ring retains 24 entries of up to 191 message characters while the
status response intentionally serializes only the newest six. Worst-case JSON
escaping makes a full response materially larger than the v2.7.9 status
response. Do not construct it in a fixed stack buffer or a one-shot oversized
HTTPS-task allocation.

Take a bounded snapshot under the existing log lock, release the lock, then
serialize and send the response in bounded chunks. Keep formatting and JSON
escaping outside the critical section. Zeroize only buffers that contain
secrets; log data is not secret storage, but the normal no-secret logging rule
continues to apply.

Expected source boundary:

- `src/management-log.c` and `include/management-log.h`: expose a bounded
  full-ring snapshot/serialization primitive while retaining the existing
  six-entry status serializer unchanged.
- New `src/management-log-routes.c` and `include/management-log-routes.h`:
  own the ADMIN full-log route and chunked response behavior.
- `src/management-routes.c`: register only the new route.
- `src/CMakeLists.txt`: explicitly register the new focused source module.
- `src/management-pages.c`: implement presentation, canonical status text,
  and click-only clipboard behavior.

`MANAGEMENT_HTTPS_ROUTE_CAPACITY` is currently 24 and its route inventory is
already 24 entries. Raise the limit to 25 for this added handler, preserve the
compile-time assertion, and do not combine this bookkeeping change with HTTP
server resource tuning.

## Browser layout and preserved behavior

- Dashboard keeps status cards but no longer owns the device-settings form.
- Device Status contains its title, `Copy JSON`, `Copy Logs`, then the device
  settings form, then the continuously visible raw status text.
- Existing tab navigation, ADMIN password, API-token, Date and Time, Wi-Fi,
  OTA, logout, session warning, and automatic refresh behavior are unchanged.
- An unauthorized status or full-log fetch returns the user to the root/login
  flow as the current page does for protected fetches.
- The full-log fetch does not refresh idle-session activity. Existing explicit
  interaction tracking remains the only session-activity mechanism.

## Acceptance matrix

| ID | Check | Required result |
| --- | --- | --- |
| UI-01 | CPS manufacturer | Visible manufacturer is `CyberPower Systems`; raw JSON remains `CPS`. |
| UI-02 | Non-CPS manufacturer | Visible and raw manufacturer remain unchanged. |
| UI-03 | Unavailable/stale UPS | Existing unavailable/stale representation remains; no cached identity appears current. |
| UI-04 | Device settings placement | Form appears only on Device Status, below title and buttons and above raw JSON. Save behavior, persistence, and hostname semantics remain unchanged. |
| UI-05 | Raw status | Raw JSON is visible after navigation and reload, with no toggle. |
| UI-06 | Copy JSON | Exactly the latest successful status response text is copied without a second request. |
| UI-07 | Copy Logs | One click issues one ADMIN full-log request and copies every entry in the returned retained window in chronological order. |
| UI-08 | Empty/full ring | Empty, one-entry, six-entry, and full 24-entry retained windows are represented accurately. Status still contains at most six entries. |
| UI-09 | Clipboard fallback | Clipboard denial/API absence exposes selectable fallback text and does not throw or navigate. |
| UI-10 | Session and authorization | Unauthenticated full-log request is `401`; bearer credentials do not authorize it; successful full-log/status GETs do not refresh idle activity. |
| UI-11 | Service regression | HTTPS `443`, read-only NUT `3493`, refused `8080`, ADMIN/CSRF on mutations, and UPS read-only policy are unchanged. |
| UI-12 | Page validity | Static validation catches embedded JavaScript syntax errors before target testing. |
| UI-13 | Responsive use | Chrome and Safari render the buttons, fallback control, form, raw JSON, and all existing tabs on MacBook and iPhone viewports. |

## Validation order

1. Static: `git diff --check`; inspect route count; parse/lint the embedded
   ADMIN-page JavaScript through `osascript -l JavaScript
   tools/validate-management-page-js.mjs` (automatically invoked by macOS
   `idf.py build`); check the full-log response schema and worst-case
   escaping/size calculations.
2. Build: clean ESP-IDF v6.0.2 `esp32s3` build; verify the new module is in
   `src/CMakeLists.txt` and that the application remains within its partition.
3. Agent-controlled target checks: authenticated ADMIN status and full-log
   requests, unauthorized/bearer rejection, one manual/automatic refresh
   request count, malformed/full/empty log cases where safely simulatable,
   443/3493/8080 boundaries, and repeated status/full-log loading while
   observing uptime, heap, and panic state.
4. Browser acceptance: Chrome and Safari on the approved direct-LAN/proxy
   paths, including clipboard behavior and session expiry.
5. Physical-world acceptance: use the checklist below. Do not publish,
   install, reset, or perform destructive hardware actions without separate
   authorization.

## Physical-world checklist for the Device Operator

These checks should be performed after the software and network checks pass.
They are deliberately conservative because this slice does not require USB,
UPS, Wi-Fi, BOOT, or firmware-recovery behavior changes.

| Step | Action | Expected evidence | Stop condition |
| --- | --- | --- | --- |
| P-01 | Leave ESP32 power, UPS USB, and network cables unchanged. Confirm the normal power/status LEDs and UPS state are unchanged. | Device stays online; UPS remains in its prior normal state. | Any unexpected reboot, LED change, UPS alarm, or loss of power. |
| P-02 | On a MacBook browser, sign in and visit Device Status. | Buttons, settings, raw JSON, and status cards render without overlap. | Login loop, missing controls, or blank/stuck status. |
| P-03 | Click `Copy JSON`; paste into a local plain-text editor. | Pasted text is valid JSON and matches the visible raw response. | Clipboard error without selectable fallback, altered/missing fields. |
| P-04 | Click `Copy Logs`; paste into a local plain-text editor. | Entries are oldest-to-newest and include more than the visible status window when the ring has more than six entries. | Missing/reordered entries, repeated page reload, or unauthorized error while signed in. |
| P-05 | Let the ADMIN session idle past its documented timeout, then click `Copy Logs`. | Expired session is handled by the normal login flow; no copied content is exposed. | Logs are returned after expiry or the page becomes unusable. |
| P-06 | On an iPhone/Safari browser, repeat P-02 through P-04. | Layout remains usable; clipboard success or visible selectable fallback is provided. | Controls are inaccessible or status/navigation regresses. |
| P-07 | Observe the device for at least 15 minutes with normal status refresh use. | No reboot, setup AP, captive portal, UPS disconnect, or unexpected LED behavior. | Any instability; report time, visible behavior, and last action. |

Do not move the UPS USB cable, press BOOT or RESET, change Wi-Fi, test OTA,
or alter power solely for this UI slice. Those actions are not acceptance
requirements and would introduce unrelated state changes. If a failure occurs,
stop at the first failure and report the exact step, browser/device, visible
message, and whether device LEDs or UPS behavior changed.

## Documentation and release closeout

- Update the development plan and current-status handoff with the final
  implementation/validation state.
- Create v2.7.10 evidence before release authorization. Record source commit,
  clean build, status/full-log authorization results, browser matrix, service
  boundaries, target stability, and this checklist's observed results.
- Use the release artifact convention: source build output
  `build/nut-esp32s3.bin`; published versioned asset
  `nut-esp32s3-vX.Y.Z.bin` with a matching `.sha256` sidecar. Record both
  paths and the verified digest.
- Archive this specification and its evidence only after an authorized
  publication. Do not record secrets, credentials, addresses, cookies, tokens,
  private keys, or Authorization headers.
