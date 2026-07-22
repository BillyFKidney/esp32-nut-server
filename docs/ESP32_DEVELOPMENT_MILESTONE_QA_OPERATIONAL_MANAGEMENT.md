# Operational Management milestone: questions and answers

**Status:** Locked for implementation on 2026-07-14. Changes that materially
expand scope or alter a recorded security decision require a new review.

**Scope:** Milestone 2, planned for the post-`v1.1.0` Operational Management
release. Production OTA design is intentionally a later milestone.

## Recorded answers

### 1. Management interface

**Question:** Should management be a browser UI, a documented JSON API, or
both?

**Answer:** Both. The expected API style is REST. The administration UI should
use that API.

### 2. Access scope

**Question:** Should management be LAN-only or support remote access?

**Answer:** LAN-only.

### 3. Authentication and roles

**Question:** What authentication and authorization model is required?

**Answer:** Use a per-device administrator password. API tokens are optional.
There will eventually be at least two roles:

- **ADMIN:** May view and edit everything.
- **USER:** May view NUT data but may not edit anything.

Milestone 2 focuses on ADMIN. USER is a later milestone.

### 4. Transport security

**Question:** Should management require HTTPS, and how should certificates be
provided?

**Answer:** Milestone 2 uses device-hosted HTTPS with a self-signed
certificate. Milestone 3 upgrades the device certificate to one issued by the
local CA.

**Security boundary:** HTTPS encrypts ADMIN passwords and API tokens from
Milestone 2. The self-signed certificate will cause a browser trust warning
until the owner explicitly trusts it; management remains LAN-only.

### 5. Initial administrator password and password recovery

**Question:** How is the first administrator password established, and how is
it recovered?

**Answer:** Approved safer flow: do not use the broadcast AP SSID as an ADMIN
secret. During first-run setup, require the owner to choose the ADMIN password
twice, verify it matches, and provide a local **Show password** control.

Physical recovery clears the ADMIN password and returns the device to this
one-time password-setup flow, so an owner cannot be permanently locked out.

### 6. Device identity

**Question:** What is the default device identity?

**Answer:** Default to `ESP32-NUT`; permit the administrator to configure the
human-facing device name.

### 7. Initial dashboard information

**Question:** What must the initial dashboard show?

**Answer:** Show firmware version, uptime, Wi-Fi SSID/signal/IP, connected UPS
identity, `ups.status`, battery/load/runtime, NUT server status, last update
result, UPS serial number, and input/output voltage. In the `v2.7.0`
diagnostics slice, also request and display the read-only NUT fields
`battery.type` (battery chemistry), `battery.mfr.date` (battery manufacturing
date), and `ups.temperature` when the driver and UPS provide them. These are
opaque NUT values; display `Not available` when the UPS omits them and retain
the existing stale-data indication rather than synthesizing values.

The same slice should show ESP32-S3 chip model, revision, core count and
feature flags; the compiled YD-ESP32-23 / ESP32-S3-WROOM-1-N16R8 board profile;
flash/PSRAM profile; free internal heap, free PSRAM, and minimum-free heap; and
the internal chip temperature, clearly labeled as chip temperature rather than
ambient or board temperature.

### 8. Wi-Fi management

**Question:** What Wi-Fi operations are required from the console?

**Answer:** Permit SSID scanning with a visible selectable list showing each
SSID, signal strength, and security mode; changing credentials; and forcing
reconnect. Manual SSID entry remains available for hidden or unlisted
networks. After selecting a scan result, collapse the list and focus the
password field; scanning again reopens the list. Never display the stored
Wi-Fi password.
The Wi-Fi password field must default to masked and provide a local **Show
password** toggle; the toggle changes only the current browser control and is
never persisted, returned by an API, or logged. A confirmation screen must
appear before committing Wi-Fi changes. The owner must also be able to clear
Wi-Fi through the physical button.

**Hardware note:** ESP32-S3 Wi-Fi is 2.4 GHz only. Its scan results should
therefore show only networks it can actually detect and join; 5 GHz-only
networks cannot be detected by this radio.

### 9. OTA policy

**Question:** What OTA controls are required?

**Answer:** Manual **check**, **download**, and **install** are mandatory.
Automatic updates are disabled by default. If enabled later, the administrator
chooses daily, weekly, or monthly checks and a time of day; a found update is
installed automatically.

The administrator must be able to select a known-good local firmware file.
Corrupt firmware must be rejected.

### 10. Remote service controls and destructive actions

**Question:** Which controls must work without a COM connection?

**Answer:** Assume the deployed board has no COM port; service controls must
be available remotely. Resetting Wi-Fi credentials or performing a factory
reset must use an explicit confirmation screen with a required acknowledgement
such as:

> By checking this box I understand that ESP32-NUT will be erased and reset to
> factory defaults. This cannot be undone.

Factory reset clears all configuration and data while retaining the current
bootable firmware and OTA recovery slot.

### 11. UPS controls

**Question:** Should UPS control be added now?

**Answer:** No. Keep UPS access read-only for this milestone. Future desired
controls include mute/silence, battery test, and display always-on.

### 12. Logging and MQTT

**Question:** What observability is required, given flash-write limits?

**Answer:** Provide live browser logs and avoid frequent or large writes to
ESP32 storage. `v2.7.0` diagnostics remain runtime-only and read-only. CPU load
must be a bounded, explicitly sampled value with the sample interval and age
available to the UI. Do not enable full FreeRTOS task runtime statistics,
enumerate task tables, or run CPU measurement in the HTTP request path solely
for this metric. Prefer a cached, on-demand or infrequent idle-time sample;
if target validation shows measurable impact on HTTPS, NUT, Wi-Fi, heap, or
watchdog behavior, return `Not available` instead. MQTT is deferred to
Milestone 6, Expanded Functionality.

### 13. Time

**Question:** How should time be managed?

**Answer:** Use `pool.ntp.org` by default, permit configuration, and provide
manual time-zone selection. Default the device to the Los Angeles time zone
(`America/Los_Angeles`). Store an IANA time-zone name so daylight-saving
transitions work correctly. Permit the administrator to set the date and time
manually when NTP is unavailable. The authenticated status API and console show
UTC and local time, the configured time zone, the time source, and whether the
clock is synchronized; an unknown clock must not be presented as a valid 1970
date.

### 14. Network integrations

**Question:** What discovery and integration work belongs now?

**Answer:** Stay with NUT for now. mDNS discovery and Home Assistant discovery
are deferred to Milestone 6, Expanded Functionality.

### 15. Physical recovery gestures

**Question:** What should the BOOT button do?

**Answer:** With ESP32-NUT already running, holding BOOT for three seconds
resets Wi-Fi. Continuing to hold BOOT for fifteen seconds requests a factory
reset, subject to the reset-scope decision; release BOOT to complete the reset
and application-controlled restart. This is not a BOOT-plus-RESET sequence,
because GPIO0 low during chip reset selects the ESP32-S3 ROM downloader.

### 16. Backup and restore

**Question:** Should configuration backup/restore be added now?

**Answer:** No. Plan it for just before the mDNS/Home Assistant work in
Milestone 6.

### 17. User experience

**Question:** What visual design is wanted now?

**Answer:** A utilitarian, mobile-friendly device-administration page. For
`v2.5.0`, the console adds a responsive, client-side tab bar across the top
with these panels: **Dashboard**, **Device Status**, **Date and Time**, **Wi-Fi
Configuration**, **ADMIN Password**, **API Tokens**, and **Update Firmware**.
Tabs are a presentation-only navigation shell; they do not create new
authentication, authorization, or transport boundaries. The default panel is
Dashboard, and the tab bar must remain usable without page-level horizontal
overflow. For `v2.7.0`, place a session-expiry warning in the same toolbar;
hide it until five minutes remain, count down the final five minutes, and
reload at zero so the expired session returns to the sign-in page.

**Observed browser-specific acceptance baseline on 2026-07-22:** when the
FQDN session expires in Chrome, a subsequent sign-in can fail with a headers
overflow error until the stale session cookie is deleted; deleting that cookie
allows login immediately. Explicit sign-out does not show this failure. Safari
can sign in after timeout without manual cookie deletion. The future session
slice must clear the expired FQDN cookie server-side and preserve both Chrome
and Safari behavior.

### 18. Definition of done

**Question:** What must be complete for Operational Management?

**Answer:** From an iPhone and MacBook Air, the owner can:

- Confirm the setup AP has a unique `ESP32-NUT-xxxx` SSID.
- Clear Wi-Fi with a physical button and reconfigure Wi-Fi in the web console
  after an explicit confirmation.
- Navigate the ADMIN console through the seven named tabs and use the Wi-Fi
  **Show password** toggle without the stored password being returned,
  persisted, or logged.
- Be required to change the ADMIN password during initial setup, and later
  change it in the console.
- Issue at least two non-expiring, uniquely named access tokens.
- See each active token's name, issue date, and final four characters.
- Delete a token only through a confirmation dialog requiring an
  acknowledgement checkbox and explicit confirmation.
- View the device's current UTC and local date/time and synchronization state,
  configure NTP and the IANA time zone, and set the clock manually when needed.
- Manually install a known-good firmware file and have a corrupt file rejected.
- View NUT battery chemistry, battery manufacturing date, and UPS temperature
  when the UPS reports them, with missing values shown as `Not available`.
- View the ESP32 chip/board profile, runtime memory, and internal chip
  temperature without exposing secrets or writing diagnostic samples to flash.
- View a bounded, sampled CPU-utilization value with its sampling age and
  interval, or an explicit `Not available` result when the low-overhead
  measurement is unsupported or fails its performance guardrail.
- See an ADMIN session countdown during the final five minutes of inactivity;
  at zero, the browser reloads and the sign-in page appears. Background
  diagnostics must not extend an idle session.
- View online diagnostics and factory-reset through the physical-button
  gesture.
- Use a REST API as it emerges from the administration console. Dedicated API
  testing is out of scope for the planned `v2.0.0` release.

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

## Resolved decisions

### Resolved question A: Management transport security options

**Question:** What are the benefits, downsides, and industry standard for
management transport security?

**Industry standard:** A device-admin interface normally uses HTTPS, a unique
administrator password, secure session cookies, CSRF protection for browser
changes, and rate limiting. API tokens are sent as bearer credentials only over
HTTPS. HTTP is appropriate only for explicitly temporary provisioning on an
isolated setup network.

| Option | Benefits | Downsides |
| --- | --- | --- |
| Device HTTPS with a certificate from a local CA | Strong direct device security; works from iPhone/Mac browsers once the CA is trusted | Requires creating/trusting a local CA and provisioning/renewing certificates |
| Device HTTPS with a self-signed certificate | Encryption without another service | Browser warnings and a poor mobile setup experience; users may train themselves to ignore certificate warnings |
| HTTP device API behind a trusted HTTPS reverse proxy | Central certificate management and polished hostname | Requires another always-on local service; the device itself still relies on trusted-LAN isolation |
| Plain HTTP on the LAN | Simplest implementation | Passwords/tokens can be intercepted or changed by another LAN client; not acceptable for the intended admin interface |

**Recorded choice:** Milestone 2 uses a self-signed certificate. Milestone 3
uses a certificate issued by the local CA.

### Resolved question B: Initial admin password

**Question:** Should the broadcast setup SSID be the initial ADMIN password,
or should initial setup require choosing the ADMIN password twice?

The requested SSID-derived password is easy to recover but is public to anyone
near the device. A safer flow is: join the temporary open AP, provision Wi-Fi,
then choose a new ADMIN password twice before the normal management interface
is enabled. A three- or fifteen-second physical recovery gesture can erase that
password and return to this one-time setup flow.

**Recorded choice:** Do not use the SSID as an administrator secret. Require a
new password during first-run setup and use the physical recovery flow to
recover from lockout.

### Resolved question C: Factory-reset scope

**Question:** On a fifteen-second BOOT hold, should “factory reset” clear all
configuration and data while retaining the current bootable firmware, or do
you intend to erase firmware too?

**Recorded choice:** Reset NVS credentials, administrator password, tokens,
device name, Wi-Fi configuration, runtime configuration, and optional logs;
retain the current bootable firmware and OTA recovery slot. Erasing firmware
would require a physical reflashing path and conflicts with cable-free
recovery.

### Deferred question D: MQTT contract

**Question:** What MQTT broker and security contract should be used?

Please specify or approve defaults for:

- Broker hostname/IP and port.
- TLS requirement and certificate strategy.
- Credentials, client certificate, or anonymous trusted-LAN access.
- Topic prefix; recommended default: `esp32-nut/<device-name>/`.
- Publication policy; recommended default: state changes plus a five-minute
  heartbeat, not high-frequency polling writes.
- Minimum payloads: device health, Wi-Fi state, OTA state, UPS summary, and
  event/log messages.

**Recorded decision:** Defer MQTT and its contract to Milestone 6, Expanded
Functionality.

### Resolved question E: Time defaults

**Question:** What default time zone and public NTP pool should be used before
the administrator configures them?

**Recorded choice:** Default to `pool.ntp.org` and
`America/Los_Angeles`, allow administrator configuration, and store an IANA
time-zone name rather than a fixed UTC offset so daylight-saving transitions
work. Provide a manual date/time fallback and expose UTC/local time, time zone,
source, and synchronization state through the authenticated API and console.

### Resolved question F: Token display and browser sessions

**Question:** May the full API token be shown exactly once when created, with
only its name, issue date, and final four characters retained/displayed
afterward? How long should ADMIN browser sessions last before reauthentication
is required?

**Recorded choice:** Show each random token once, store only a salted verifier,
and require reauthentication after a configurable but short idle interval.

### Resolved question G: HTTPS milestone boundary

**Question:** How is HTTPS divided between Operational Management and
Production OTA?

**Recorded choice:** Milestone 2 establishes device-hosted HTTPS with a
self-signed certificate for the LAN-only administration interface. Milestone 3
replaces that certificate with one issued by the local CA, alongside production
OTA hardening. Management is never to be exposed outside the trusted LAN.

### Resolved question H: ADMIN console navigation

**Question:** May the existing ADMIN console sections be organized into a
phpMyAdmin-style tab bar, and does that change any security boundary?

**Recorded choice:** Yes. The Project Maintainer requested the tabbed layout
on 2026-07-21. Implement it as a client-side tab shell in the existing
authenticated page. Use the labels **Dashboard**, **Device Status**, **Date and
Time**, **Wi-Fi Configuration**, **ADMIN Password**, **API Tokens**, and
**Update Firmware**. Tabs do not add routes, sessions, roles, cookies, or
transport modes; every panel remains protected by the existing ADMIN session,
and state-changing actions retain their existing CSRF requirements.

### Resolved question I: v2.7 diagnostics performance and session expiry

**Question:** How should v2.7 expose CPU load and session expiry without
turning diagnostics into a performance or security regression?

**Recorded choice:** Keep diagnostics runtime-only, read-only, and bounded.
Do not enable full FreeRTOS task runtime statistics solely to produce a
dashboard percentage. Prefer a cached, on-demand or infrequent idle-time
sample, never run it in the HTTP request path, expose the sample age and
interval, and show `Not available` if target validation finds measurable
impact. The existing fifteen-minute server-side idle timeout remains the
authority. Show a toolbar countdown only during its final five minutes; normal
administrator actions may refresh the deadline, background diagnostic polling
must not, and expiration or an authenticated `401`/`403` reloads the page to
the sign-in screen.
