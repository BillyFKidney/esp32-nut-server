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

**Answer:** Show firmware version, uptime, Wi-Fi signal/IP, connected UPS
identity, `ups.status`, battery/load/runtime, NUT server status, last update
result, UPS serial number, and input/output voltage.

### 8. Wi-Fi management

**Question:** What Wi-Fi operations are required from the console?

**Answer:** Permit SSID scanning, signal-strength display, changing
credentials, and forcing reconnect. Never display the stored Wi-Fi password.
A confirmation screen must appear before committing Wi-Fi changes. The owner
must also be able to clear Wi-Fi through the physical button.

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
ESP32 storage. MQTT is deferred to Milestone 6, Expanded Functionality.

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

**Answer:** A utilitarian, mobile-friendly device-administration page. UX
improvements are much later work.

### 18. Definition of done

**Question:** What must be complete for Operational Management?

**Answer:** From an iPhone and MacBook Air, the owner can:

- Confirm the setup AP has a unique `ESP32-NUT-xxxx` SSID.
- Clear Wi-Fi with a physical button and reconfigure Wi-Fi in the web console
  after an explicit confirmation.
- Be required to change the ADMIN password during initial setup, and later
  change it in the console.
- Issue at least two non-expiring, uniquely named access tokens.
- See each active token's name, issue date, and final four characters.
- Delete a token only through a confirmation dialog requiring an
  acknowledgement checkbox and explicit confirmation.
- View the device's current UTC and local date/time and synchronization state,
  configure NTP and the IANA time zone, and set the clock manually when needed.
- Manually install a known-good firmware file and have a corrupt file rejected.
- View online diagnostics and factory-reset through the physical-button
  gesture.
- Use a REST API as it emerges from the administration console. Dedicated API
  testing is out of scope for the planned `v2.0.0` release.

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
