# ESP32-NUT development plan

## Purpose

This document tracks the downstream ESP32-S3 work in this repository. It is a
roadmap, not a replacement for upstream Network UPS Tools (NUT) documentation.
The project extends the existing `banoz/nut` ESP32 port and preserves its
connected upstream NUT history.

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
| Development OTA baseline | In progress | Dual OTA slots and rollback are enabled; a Wi-Fi upload to the inactive slot was validated on-device. |

## Current development OTA milestone

The current `feature/development-ota` work adds a development-only OTA HTTP
server after station Wi-Fi connects.

- `GET http://<device-ip>:8080/` reports the running and next OTA slots.
- `POST http://<device-ip>:8080/ota` accepts a complete ESP-IDF application
  image, verifies it, selects the inactive slot, and restarts.
- ESP-IDF rollback marks a newly booted image valid after core services start;
  a boot failure before then reverts to the previous image.
- The first full OTA update was validated from `app0` to `app1`, followed by
  Wi-Fi, NUT TCP, and CyberPower `ups.status = OL` checks.

This endpoint is intentionally unauthenticated for trusted-LAN development.
It must not be treated as a production update mechanism.

## Next milestones

### 1. Finish the development OTA feature

- Review the local implementation and documentation.
- Commit it on `feature/development-ota`, open a pull request, and merge with
  a merge commit after validation.
- Release a post-1.0 version only after the intended release scope is chosen.

Success criterion: a clean rebuild can update the deployed board repeatedly
over Wi-Fi, alternate OTA slots, and preserve Wi-Fi/UPS/NUT operation.

### 2. Production OTA design

- Replace the unauthenticated LAN upload endpoint with an authenticated update
  policy.
- Use HTTPS with certificate validation for an update source.
- Verify a signed firmware manifest or enable an ESP-IDF-supported signed-image
  and secure-boot strategy before enabling unattended updates.
- Define a release-asset and version-manifest workflow, preferably tied to
  GitHub releases or a controlled update service.
- Decide whether updates are manually approved, scheduled, or automatically
  installed.

Success criterion: an unauthorized LAN client and a compromised download path
cannot cause firmware installation; a failed approved image rolls back.

### 3. Operational management

- Add a deliberate administrative status/configuration interface once its
  authentication model is defined.
- Expose device firmware version, Wi-Fi state, UPS identity, uptime, and OTA
  status without exposing credentials.
- Add a supported way to reset configuration and recover from a failed Wi-Fi
  deployment.

Success criterion: a maintainer can diagnose and recover a device remotely
without a serial cable under normal network conditions.

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

## Change tracking conventions

- Use one feature branch and pull request per milestone or independently
  reviewable change.
- Prefer merge commits when merging into `main` to preserve upstream and
  downstream history.
- Record hardware validation in the pull request description and relevant
  ESP32 documentation.
- Update this plan when a milestone materially changes status or scope.
