# ESP32-NUT development plan

## Purpose

This document tracks the downstream ESP32-S3 work in this repository. It is a
roadmap, not a replacement for upstream Network UPS Tools (NUT) documentation.
The project extends the existing `banoz/nut` ESP32 port and preserves its
connected upstream NUT history.

Current repository, firmware, hardware-validation, and connection state is
recorded in [ESP32_CURRENT_STATUS.md](ESP32_CURRENT_STATUS.md).

## Project guardrails

- Keep the existing NUT daemon and driver architecture. Do not create a
  separate NUT protocol server or standalone USB-enumeration implementation.
- Preserve upstream structure and make downstream changes small, documented,
  and easy to rebase or compare with `banoz/nut`.
- Keep UPS control disabled until it has a separately reviewed safety model.
  Current NUT access is read-only.
- Do not track generated ESP-IDF build output, managed components, dependency
  locks, local `sdkconfig`, or machine/editor settings.
- Validate changes on the ESP32-S3 target before considering them complete.

## Target platform

| Item | Current target |
| --- | --- |
| Board | YD-ESP32-23 |
| Module | ESP32-S3-WROOM-1-N16R8 |
| SDK | ESP-IDF v6.0.2 |
| Flash / PSRAM | 16 MB flash / 8 MB octal PSRAM |
| UPS connection | Native USB host port |
| Development console | COM USB serial port |

## Completed milestones

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
The Operational Management branch retires it in favor of an authenticated
HTTPS management route; it must never be restored as a production mechanism.

## Next milestones

### 1. Finish the development OTA feature — Complete

Released as `v1.1.0`. A clean ESP-IDF v6.0.2 build was installed over Wi-Fi,
alternated OTA slots, restarted automatically, and preserved Wi-Fi, NUT, and
CyberPower UPS monitoring.

### 2. Operational management — Locked for implementation

The detailed requirements and recorded decisions are in
[ESP32_DEVELOPMENT_MILESTONE_QA_OPERATIONAL_MANAGEMENT.md](ESP32_DEVELOPMENT_MILESTONE_QA_OPERATIONAL_MANAGEMENT.md).
This milestone is the planned `v2.0.0` scope.

#### Intended scope

- A utilitarian mobile-friendly browser administration console backed by an
  emerging REST API. Dedicated API testing is out of scope for `v2.0.0`.
- LAN-only ADMIN access with per-device password authentication. USER role is
  explicitly deferred. This milestone uses device-hosted HTTPS with a
  self-signed certificate.
- Administrator password setup/change/recovery, including twice-entered
  password fields and a show-password control.
- Named non-expiring API tokens: issue at least two; display name, issue date,
  and final four characters; require an acknowledgement and explicit
  confirmation to delete.
- Dashboard/device diagnostics: firmware version, uptime, Wi-Fi IP/signal,
  UPS identity and serial number, `ups.status`, battery/load/runtime,
  input/output voltage, NUT service status, and last update result.
- Wi-Fi scan and configuration with signal strength, only 2.4 GHz-capable scan
  results, no stored-password display, and a confirmation before committing
  changes.
- Manual firmware file selection with check/download/install controls;
  known-good images install and corrupt images are rejected.
- Remote service controls, live browser diagnostics, public NTP via
  `pool.ntp.org` with configurable manual time zone (default
  `America/Los_Angeles`), and physical recovery.
- BOOT gestures: three seconds clears Wi-Fi; fifteen seconds factory-resets
  the agreed configuration scope.
- No UPS controls in this milestone.

#### Implementation slices

Operational Management is an umbrella milestone, not a single long-running
feature branch. Deliver it through small branches that begin at the latest
`main`, have one coherent acceptance boundary, and merge independently after
proportional build and target-hardware validation.

| Order | Branch | Scope and merge boundary |
| --- | --- | --- |
| 1 | `feature/operational-management` | HTTPS and ADMIN authentication foundation: device certificate, initial password setup, secure browser session, CSRF and login throttling, initial status/OTA routes, and stack-safe startup. Delivered by PR #10. |
| 2 | `feature/admin-password-management` | Complete and validate initial setup, password change, session expiration, login throttling, and physical password recovery. |
| 3 | `feature/api-tokens` | Create, list, identify, and delete named non-expiring API tokens with the required confirmation flow. |
| 4 | `feature/management-dashboard` | Expose and render the required firmware, Wi-Fi, NUT, UPS, voltage, battery, load, runtime, and update diagnostics. |
| 5 | `feature/wifi-management` | Scan supported networks, show signal strength, confirm credential changes, reconnect, and never reveal the stored password. |
| 6 | `feature/local-ota-management` | Add the management UI and validation flow for known-good local firmware while rejecting corrupt images. |
| 7 | `feature/live-diagnostics` | Add bounded live browser logs and reviewed remote service controls without high-frequency flash writes. |
| 8 | `feature/time-configuration` | Add configurable NTP and IANA time-zone management with the recorded defaults. |
| 9 | `feature/physical-recovery` | Complete and validate the three-second Wi-Fi reset and fifteen-second factory-reset behavior and scope. |
| 10 | `feature/operational-management-acceptance` | Integrate and validate the definition of done from iPhone and MacBook Air, close documentation gaps, and prepare the `v2.0.0` release candidate. |

The console and REST API are cross-cutting design requirements applied to each
slice rather than separate branches. A slice may be split further when review
or hardware evidence shows that it has more than one independent risk boundary.
Merging an implementation slice does not mark the umbrella milestone complete;
that happens only after the acceptance slice satisfies the locked definition of
done.

#### Lock criteria

The requirements in the linked Q&A were locked on 2026-07-14. Management
remains LAN-only; Milestone 3 replaces the self-signed certificate with one
issued by the local CA. Material scope or security changes require another
review before implementation.

Success criterion: from an iPhone or MacBook Air, an owner can safely manage
Wi-Fi, ADMIN credentials, named tokens, diagnostics, local firmware updates,
and recovery without a COM connection.

### 3. Production OTA design

- Build on the authenticated HTTPS local-upload route delivered by Operational
  Management; do not restore the unauthenticated development LAN endpoint.
- Replace the self-signed device certificate with one issued by the local CA,
  and harden certificate trust/management.
- Use HTTPS with certificate validation for an update source.
- Verify a signed firmware manifest or enable an ESP-IDF-supported signed-image
  and secure-boot strategy before enabling unattended updates.
- Define a release-asset and version-manifest workflow, preferably tied to
  GitHub releases or a controlled update service.
- Support manual check/download/install first; keep automatic installation
  disabled by default. When enabled, let ADMIN select daily, weekly, or
  monthly checks and a time of day.

Success criterion: an unauthorized LAN client and a compromised download path
cannot cause firmware installation; a failed approved image rolls back.

### 4. NUT and UPS compatibility hardening

- Test additional CyberPower devices and other USB HID UPS models.
- Improve descriptor/driver diagnostics and compatibility selection where
  evidence shows it is needed.
- Verify NUT client interoperability (`upsc`, Home Assistant/NUT clients, and
  monitoring systems) against the read-only server.
- Keep write/control paths blocked until an explicit UPS-control milestone.

Success criterion: documented supported-device behavior and stable read-only
monitoring across the tested clients.

### 5. Platform resilience and release automation

- Decide whether to expand the current lower-8-MB partition layout to use the
  full 16-MB flash; preserve dual OTA capacity when doing so.
- Add automated ESP-IDF builds and release artifacts for the exact ESP32-S3
  target.
- Record repeatable hardware-validation steps and release checks.
- Evaluate secure boot, flash encryption, and certificate storage for any
  non-development deployment.

Success criterion: reproducible release artifacts, documented recovery paths,
and a validated upgrade/downgrade policy.

### 6. Expanded functionality

Defer the following capabilities until the Operational Management and
Production OTA security foundations are complete:

- MQTT, including broker/security/topic contract and bounded publication
  cadence.
- Home Assistant discovery and integration.
- mDNS discovery.
- Read-only USER role.
- Explicitly reviewed UPS controls: mute/silence, display always-on, and
  battery test.
- Configuration backup/restore immediately before the mDNS/Home Assistant
  work, excluding Wi-Fi credentials and administrator secrets by default.

Success criterion: each capability has its own security and hardware-safety
review, especially any UPS write/control operation.

## Change tracking conventions

- Use one feature branch and pull request per independently reviewable slice.
  Large milestones must be divided into small slices with explicit merge
  boundaries.
- Prefer merge commits when merging into `main` to preserve upstream and
  downstream history.
- Record hardware validation in the pull request description and relevant
  ESP32 documentation.
- Update this plan when a milestone materially changes status or scope.
