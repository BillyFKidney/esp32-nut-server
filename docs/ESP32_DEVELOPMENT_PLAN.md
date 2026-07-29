# ESP32-NUT development plan

## Purpose

This document tracks the downstream ESP32-S3 work in this repository. It is a
roadmap, not a replacement for upstream Network UPS Tools (NUT) documentation.
The project extends the existing `banoz/nut` ESP32 port and preserves its
connected upstream NUT history.

Current repository, firmware, hardware-validation, and connection state is
recorded in [ESP32_CURRENT_STATUS.md](ESP32_CURRENT_STATUS.md).

## Milestone, slice, and version model

ESP32-NUT uses milestone-oriented product versioning while its management API
is still emerging; it does not claim strict Semantic Versioning compatibility
for that unstable API.

- A numbered **milestone** is an umbrella major-version release family such as
  Operational Management `v2.x`. It is never one long-running branch.
- An **implementation slice** is one independently reviewable feature branch,
  pull request, acceptance boundary, and prospective minor release.
- `MAJOR` identifies the umbrella milestone, `MINOR` identifies a validated and
  published slice within it, and `PATCH` is reserved for compatible fixes to a
  published slice.
- A branch merge does not consume a version by itself. Publish the assigned
  version only when that slice is validated, accepted, and explicitly
  authorized for release; use `-rc.N` for release candidates when needed.
- Each milestone ends with a combined acceptance slice. Completing or merging
  earlier slices does not declare the umbrella milestone complete.

## Project guardrails

- Keep the existing NUT daemon and driver architecture. Do not create a
  separate NUT protocol server or standalone USB-enumeration implementation.
- Preserve the required upstream source and build structure while keeping
  application procedures and retained upstream narrative documents in the
  directories defined by [ESP32_REPOSITORY_LAYOUT.md](ESP32_REPOSITORY_LAYOUT.md).
  Make downstream changes small, documented, and easy to rebase or compare
  with `banoz/nut`.
- Keep UPS control disabled until it has a separately reviewed safety model.
  Current NUT access is read-only.
- Do not track generated ESP-IDF build output, managed components, dependency
  locks, local `sdkconfig`, or machine/editor settings.
- Validate changes on the ESP32-S3 target before considering them complete.

## Current engineering-maintenance track: modular refactoring

The large downstream orchestration units are being reduced through small,
behavior-preserving extraction branches. The staged boundaries, security
invariants, and validation rules are recorded in
[ESP32_REFACTORING_PLAN.md](ESP32_REFACTORING_PLAN.md). This track does not
consume a release version by itself and must not blur the acceptance boundary
of the active UPS-state or factory-reset slices.

The first branch is `feature/management-log-module`, which isolates bounded
in-memory management log capture and status-response serialization. The second
branch is `feature/management-status-module`, which isolates only read-only NUT
and hardware snapshots while retaining the authenticated route and JSON
assembly in `management.c`. The active follow-on branch is
`feature/management-certificates-module`, which isolates the persisted
self-signed HTTPS certificate and private-key lifecycle while keeping HTTPS
startup and routing in `management.c`. Credentials, sessions/CSRF, and Wi-Fi
recovery remain separate later boundaries because they carry security,
persistence, or physical-recovery risk.

## Target platform

| Item | Current target |
| --- | --- |
| Board | YD-ESP32-23 |
| Module | ESP32-S3-WROOM-1-N16R8 |
| SDK | ESP-IDF v6.0.2 |
| Flash / PSRAM | 16 MB flash / 8 MB octal PSRAM |
| UPS connection | Native USB host port |
| Development console | COM USB serial port |

## Active and future milestones

Completed foundation, development-OTA, release, and acceptance history is in
[ESP32_DEVELOPMENT_PLAN_COMPLETED_HISTORY.md](archive/ESP32_DEVELOPMENT_PLAN_COMPLETED_HISTORY.md).
The retired unauthenticated development listener on TCP `8080` must not be
restored; the current authenticated update paths and workflow-continuity rules
remain authoritative.

### 2. Operational management — `v2.x`, locked for implementation

The detailed requirements and recorded decisions are in
[ESP32_DEVELOPMENT_MILESTONE_QA_OPERATIONAL_MANAGEMENT.md](ESP32_DEVELOPMENT_MILESTONE_QA_OPERATIONAL_MANAGEMENT.md).
This milestone is the `v2.x` release family.

#### Intended scope

- A utilitarian mobile-friendly browser administration console backed by an
  emerging REST API. Dedicated API testing is out of scope for `v2.x`.
- The ADMIN console uses a responsive, client-side tab bar with these panels:
  Dashboard, Device Status, Date and Time, Wi-Fi Configuration, ADMIN
  Password, API Tokens, and Update Firmware. Tabs are presentation-only and do
  not create new authentication, authorization, or transport boundaries.
- LAN-only ADMIN access with per-device password authentication. USER role is
  explicitly deferred. This milestone uses device-hosted HTTPS with a
  self-signed certificate.
- Administrator password setup/change/recovery, including twice-entered
  password fields and a show-password control.
- Named non-expiring API tokens: issue at least two; display name, issue date,
  and final four characters; require an acknowledgement and explicit
  confirmation to delete.
- Dashboard/device diagnostics: firmware version, uptime, Wi-Fi SSID/IP/signal,
  UPS identity and serial number, `ups.status`, battery/load/runtime,
  input/output voltage, NUT service status, and last update result. Expand the
  read-only NUT snapshot with `battery.type` (battery chemistry),
  `battery.mfr.date` (battery manufacturing date), and `ups.temperature` when
  the driver and UPS provide them; show `Not available` rather than inventing
  values when the UPS does not populate a field. Also expose the ESP32 chip
  model/revision/core/features, the YD-ESP32-23 board profile, flash/PSRAM
  profile, internal/PSRAM free heap, minimum-free heap, and chip temperature.
  CPU load is a bounded, explicitly sampled diagnostic with its sampling
  interval shown; it must not introduce high-frequency polling, frequent flash
  writes, or a measurable service-performance regression.
- Wi-Fi scan and configuration with a visible selectable list of only 2.4
  GHz-capable results, including SSID, signal strength, and security mode;
  manual entry for hidden or unlisted networks; no stored-password display; a
  local `Show password` toggle that is off by default and never persisted; and
  a confirmation before committing changes. Selecting a result collapses the
  list and focuses the password field; scanning again reopens the list.
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

| Order | Release target | Branch | Scope and merge boundary |
| --- | --- | --- | --- |
| 1–8 | `v2.0.0`–`v2.7.0` | Completed branches | Published slice details and acceptance evidence are archived in the completed-history document linked above. |
| 9 | `v2.8.0` | `feature/physical-recovery` | Complete and validate the three-second Wi-Fi reset and fifteen-second factory-reset behavior and scope. |
| 10 | `v2.9.0` | `feature/operational-management-acceptance` | Integrate and validate the definition of done from iPhone and MacBook Air, close documentation gaps, and publish the final `v2.x` acceptance release. |

The completed v2.5.0/v2.7.0 planning rationale, build provenance, and target
acceptance records are preserved in the same completed-history archive.

### v2.7.1-v2.7.6 UPS state and compatibility fixes

**Reported by the Project Maintainer on 2026-07-22 after v2.7.0 acceptance:**
the following defects are immediately prioritized. These observations are
recorded as evidence; the underlying NVS/runtime-state and USB/NUT failure
causes remain to be confirmed during implementation.

- Disconnecting the CyberPower UPS leaves the last UPS identity and values
  visible while NUT reports `STALE`/data stale. The values are not invalidated.
- A 15-second-plus factory reset with the UPS disconnected does not clear the
  displayed UPS identity or values. It is not yet proven whether these values
  are persisted in NVS or retained in a runtime cache across the reset path.
- Connecting an APC Back-UPS RS 1500G (`BR1500G`) after healthy communication
  through the MacMini causes the ESP32 to freeze and reboot repeatedly. This
  is a confirmed compatibility/recovery symptom, not yet a confirmed parser,
  driver, resource, or USB-host root cause. Reference:
  [APC BR1500G product page](https://www.se.com/us/en/product/BR1500G/apc-backups-pro-1500va-865w-tower-120v-10x-nema-515r-outlets-avr-lcd-user-replaceable-battery/).

Each patch release below is one reviewable branch and acceptance boundary:

| Release | Prospective branch | Required outcome |
| --- | --- | --- |
| `v2.7.1` | `feature/factory-reset-clears-state` | A 15-second-plus factory reset erases all defined saved user values, including UPS identity/cache state, while preserving the firmware and documented recovery boundary. |
| `v2.7.2` | `feature/nut-disconnect-invalidation` | Loss of the UPS connection immediately updates the status JSON and dashboard; stale data is never presented as current UPS information. |
| `v2.7.3` | `feature/nut-stale-timeout` | After five minutes without a connection, all UPS information is cleared while NUT remains explicitly stale/unavailable. The timer resets only after a confirmed connection. |
| `v2.7.4` | `feature/apc-br1500g-support` | From factory-reset state, the APC Back-UPS RS 1500G communicates without freeze/reboot and reports a validated NUT identity, status, and available measurements. |
| `v2.7.5` | `feature/nut-compatibility-hardening` | Expand compatibility handling for NUT-compatible UPS devices with graceful unsupported-device behavior, bounded resources, and a documented validation matrix; do not claim universal support without evidence. |
| `v2.7.6` | `feature/ups-change-without-wait` | A connected UPS can be replaced and reprobed immediately without waiting for the five-minute timeout; old identity and values cannot leak into the new device state. |

The first implementation slice is `v2.7.1`; reproduce the disconnected-UPS
factory-reset case and define the persistent-state boundary before touching
APC-specific compatibility work.

Historical sequencing, workflow-continuity rationale, and publication records
are preserved in the completed-history archive.

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

### 3. Production OTA — `v3.x`

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

#### Implementation slices

| Order | Release target | Prospective branch | Scope and merge boundary |
| --- | --- | --- | --- |
| 1 | `v3.0.0` | `feature/local-ca-trust` | Replace the self-signed management certificate with the reviewed local-CA trust and provisioning model. |
| 2 | `v3.1.0` | `feature/signed-update-metadata` | Define and verify signed release metadata or an ESP-IDF-supported signed-image strategy. |
| 3 | `v3.2.0` | `feature/remote-update-client` | Check, download, and manually approve a remote release through certificate-validated HTTPS. |
| 4 | `v3.3.0` | `feature/scheduled-updates` | Add opt-in daily, weekly, or monthly scheduling while keeping automatic installation disabled by default. |
| 5 | `v3.4.0` | `feature/production-ota-acceptance` | Validate authorization, compromised-source resistance, rollback, recovery, and the complete production OTA definition of done. |

Lock detailed requirements and revisit slice boundaries before beginning
`v3.0.0`; split security-sensitive slices further when their review or rollback
boundaries differ.

### 4. NUT and UPS compatibility hardening — `v4.x`

- Test additional CyberPower devices and other USB HID UPS models.
- Improve descriptor/driver diagnostics and compatibility selection where
  evidence shows it is needed.
- Verify NUT client interoperability (`upsc`, Home Assistant/NUT clients, and
  monitoring systems) against the read-only server.
- Keep write/control paths blocked until an explicit UPS-control milestone.

Success criterion: documented supported-device behavior and stable read-only
monitoring across the tested clients.

#### Implementation slices

| Order | Release target | Prospective branch | Scope and merge boundary |
| --- | --- | --- | --- |
| 1 | `v4.0.0` | `feature/nut-client-interoperability` | Validate `upsc`, Home Assistant/NUT clients, and representative monitoring systems against the read-only server. |
| 2 | `v4.1.0` | `feature/cyberpower-compatibility` | Test additional available CyberPower devices and document supported behavior and differences. |
| 3 | `v4.2.0` | `feature/usb-hid-compatibility` | Improve bounded descriptor diagnostics and driver selection for evidenced USB HID compatibility gaps. |
| 4 | `v4.3.0` | `feature/nut-ups-acceptance` | Consolidate the supported-device/client matrix and validate sustained read-only operation without adding UPS controls. |

Hardware-dependent slices may remain pending until the required models are
available; lack of hardware does not authorize unvalidated compatibility
claims.

### 5. Platform resilience and release automation — `v5.x`

- Decide whether to expand the current lower-8-MB partition layout to use the
  full 16-MB flash; preserve dual OTA capacity when doing so.
- Add automated ESP-IDF builds and release artifacts for the exact ESP32-S3
  target.
- Record repeatable hardware-validation steps and release checks.
- Evaluate secure boot, flash encryption, and certificate storage for any
  non-development deployment.

Success criterion: reproducible release artifacts, documented recovery paths,
and a validated upgrade/downgrade policy.

#### Implementation slices

| Order | Release target | Prospective branch | Scope and merge boundary |
| --- | --- | --- | --- |
| 1 | `v5.0.0` | `feature/flash-layout` | Decide and validate whether to expand the application layout while preserving dual OTA and recovery. |
| 2 | `v5.1.0` | `feature/release-automation` | Add exact-target CI builds, artifact provenance, and an explicitly authorized publication workflow. |
| 3 | `v5.2.0` | `feature/upgrade-recovery-policy` | Validate repeatable installation, rollback, upgrade, downgrade, and physical recovery procedures. |
| 4 | `v5.3.0` | `feature/platform-security` | Review and, where approved, implement secure boot, flash encryption, and hardened certificate storage as separable risk boundaries. |
| 5 | `v5.4.0` | `feature/platform-acceptance` | Exercise the release, security, resource, and recovery definition of done on the exact target hardware. |

### 6. Expanded functionality — `v6.x`

Defer the following capabilities until the Operational Management and
Production OTA security foundations are complete:

- MQTT, including broker/security/topic contract and bounded publication
  cadence.
- Home Assistant discovery and integration.
- mDNS discovery.
- Password UX experience
- Read-only USER role.
- Explicitly reviewed UPS controls: mute/silence, display always-on, and
  battery test.
- Configuration backup/restore immediately before the mDNS/Home Assistant
  work, excluding Wi-Fi credentials and administrator secrets by default.

Success criterion: each capability has its own security and hardware-safety
review, especially any UPS write/control operation.

#### Prospective implementation slices

These requirements remain deferred and are not locked. Confirm their order,
scope, and version assignments before beginning `v6.0.0`.

| Order | Release target | Prospective branch | Scope and merge boundary |
| --- | --- | --- | --- |
| 1 | `v6.0.0` | `feature/config-backup-restore` | Add reviewed configuration backup/restore before discovery and Home Assistant work, excluding secrets by default. |
| 2 | `v6.1.0` | `feature/password-ux` | Improve ADMIN password mismatch and related field-specific UX without weakening server-side validation. |
| 3 | `v6.2.0` | `feature/mqtt` | Lock and implement the broker, security, topic, reconnect, and bounded-publication contract. |
| 4 | `v6.3.0` | `feature/mdns-discovery` | Add reviewed LAN discovery without broadening management exposure. |
| 5 | `v6.4.0` | `feature/home-assistant-integration` | Add Home Assistant discovery and validate entity behavior against the read-only operating model. |
| 6 | `v6.5.0` | `feature/user-role` | Add the read-only USER role with explicit authorization and recovery behavior. |
| 7 | `v6.6.0` | `feature/ups-control-safety` | Complete and lock the separate UPS-control hazard, authorization, and recovery review before enabling writes. |
| 8 | `v6.7.0` | `feature/ups-controls` | Implement only the explicitly approved mute/display/test controls with model-specific hardware validation. |
| 9 | `v6.8.0` | `feature/expanded-functionality-acceptance` | Validate the combined `v6.x` security, integration, hardware-safety, recovery, and supported-client definition of done. |

## Change tracking conventions

- Use one feature branch and pull request per independently reviewable slice.
  Large milestones must be divided into small slices with explicit merge
  boundaries.
- Prefer merge commits when merging into `main` to preserve upstream and
  downstream history.
- Record hardware validation in the pull request description and relevant
  ESP32 documentation.
- Update this plan when a milestone materially changes status or scope.
- Never retire a service without explicit Project Maintainer approval naming
  that service, even when retirement is recommended for security.
- Before a change removes Agent access, independent update/deployment ability,
  or automation—or makes a previously Agent-owned task a human responsibility—
  document the before/after workflow, replacement, validation, and rollback,
  then obtain explicit Project Maintainer approval.
