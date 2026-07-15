# ESP32-NUT current development status

This document is the operational handoff for the active ESP32-S3 development
session. It complements the long-term roadmap in
[ESP32_DEVELOPMENT_PLAN.md](ESP32_DEVELOPMENT_PLAN.md); it does not replace it.

Before interacting with hardware, use [ESP32_PREFLIGHT.md](ESP32_PREFLIGHT.md).
Development-team authority and responsibilities are defined in
[ESP32_DEVELOPMENT_ROLES.md](ESP32_DEVELOPMENT_ROLES.md).

Update this file whenever a session changes the repository, installed firmware,
device connectivity, or next action. Record observations with a timestamp and
mark assumptions explicitly. Never put passwords, session cookies, API tokens,
private keys, or Wi-Fi credentials here.

## Snapshot

| Field | Value |
| --- | --- |
| Updated | 2026-07-15, America/Los_Angeles |
| Active milestone | Operational Management, planned for `v2.0.0` |
| Repository branch | `feature/operational-management` |
| Validated implementation commit | `0fcd9e1f9` (`fix: start HTTPS management outside system event task`) |
| Remote state | `feature/operational-management` synchronized through the 2026-07-15 documentation checkpoint; verify before new work |
| Source worktree | Expected clean after the checkpoint except ignored generated files and the retained serial diagnostic artifact |
| Build environment | ESP-IDF v6.0.2, target `esp32s3` |
| Latest local build | `0fcd9e1f9` builds successfully; application image is about 1.2 MB with 63% of the smallest application partition free |
| Board | YD-ESP32-23 with ESP32-S3-WROOM-1-N16R8 |
| UPS | CyberPower CST150UC2 on the ESP32 native USB host port |
| Last verified IPv4 address | `192.168.40.173` on 2026-07-15; verify with UniFi at the start of a new session |
| Last observed COM device | `/dev/cu.usbmodem54E20396741` on 2026-07-15; discover it again rather than hard-coding it |
| Physical intervention required | None at the end of the previous session |

## Current objective

Deliver the locked Operational Management milestone: a LAN-only, mobile-friendly
HTTPS administration console and emerging REST API with ADMIN authentication,
diagnostics, Wi-Fi management, named API tokens, authenticated local OTA, and
physical recovery. UPS access remains read-only.

The authoritative scope and security decisions are in
[ESP32_DEVELOPMENT_MILESTONE_QA_OPERATIONAL_MANAGEMENT.md](ESP32_DEVELOPMENT_MILESTONE_QA_OPERATIONAL_MANAGEMENT.md).

## Last completed work

- Replaced the development HTTP OTA listener on TCP port 8080 with an HTTPS
  management foundation on TCP port 443.
- Added a device-generated self-signed certificate stored in management NVS.
- Added first-run ADMIN password setup. The password is stored as a salted
  PBKDF2-HMAC-SHA-256 verifier, not as plaintext.
- Added ADMIN login, a fifteen-minute idle session, Secure/HttpOnly/SameSite
  cookies, CSRF checks for state-changing routes, and failed-login throttling.
- Added an initial authenticated status API and an authenticated OTA install
  route.
- Added management-data erasure to the fifteen-second BOOT factory-reset path.
- Fixed a system event-task stack overflow by scheduling HTTPS initialization
  in its own FreeRTOS task.
- Built the stack-safety fix, installed it through OTA, and committed the tested
  working-tree change as `0fcd9e1f9`.

## Installed firmware and hardware evidence

The board was last verified running the working-tree build that was subsequently
committed as `0fcd9e1f9`. Its reported version was
`v1.1.0-4-ged37c1f7c-dirty` because the fix had not yet been committed when the
image was built. This is expected and does not by itself require reflashing.

The 2026-07-15 serial log confirmed:

- Wi-Fi received `192.168.40.173`.
- A device-specific self-signed certificate was generated.
- HTTPS began listening on TCP port 443 and completed a TLS handshake.
- USB UPS discovery found the CyberPower CST150UC2.
- The read-only NUT service remained operational on TCP port 3493 with
  `ups.status = OL`.
- The bootloader marked the OTA image valid, so rollback is no longer pending.
- No physical reset was needed, and the COM port was released at session end.

The earlier failing image overflowed the ESP-IDF `sys_evt` task while starting
HTTPS. That failure is the reason for commit `0fcd9e1f9`; do not move expensive
certificate or HTTPS startup work back into the system event callback.

## Implemented versus remaining

### Implemented foundation

- Self-signed device HTTPS on TCP port 443
- Initial ADMIN password creation and sign-in
- Browser session, CSRF, and login-throttling foundation
- Initial status JSON endpoint
- Authenticated local OTA installation endpoint
- Three-second Wi-Fi reset and fifteen-second management factory reset
- Read-only NUT service on TCP port 3493
- USB HID polling for the validated CyberPower UPS

### Remaining Operational Management work

- Complete browser validation of ADMIN setup, login, logout, CSRF enforcement,
  idle expiration, and rate limiting
- ADMIN password-change workflow
- Named API-token creation, display, and confirmed deletion
- Full dashboard data, including UPS identity, serial number, status, battery,
  load, runtime, input/output voltage, NUT health, and last update result
- Wi-Fi scan, signal display, credential change, confirmation, and reconnect
- OTA file-selection UI and complete known-good/corrupt-image validation
- Remote service controls and live browser diagnostics
- NTP and IANA time-zone configuration
- Full three-second and fifteen-second physical recovery validation
- iPhone and MacBook Air acceptance testing

## Exact next action

1. Confirm the board's current IP in UniFi and verify HTTPS on TCP 443 and NUT
   on TCP 3493 without opening the serial port.
2. Exercise first-run ADMIN setup and authentication behavior in a browser.
3. Implement ADMIN password management and named API tokens as the next
   independently reviewable feature slice.

## Operational procedures

- Start and end hardware sessions with
  [ESP32_PREFLIGHT.md](ESP32_PREFLIGHT.md).
- Use [ESP32_DEVELOPMENT_ROLES.md](ESP32_DEVELOPMENT_ROLES.md) to identify who
  can observe, authorize, perform, or accept each action.
- Keep procedural checklists in those files and keep this document focused on
  changing project state and the exact next action.

## Diagnostic artifacts

- `cleanup/artifacts/serial/2026-07-15-https-foundation-screenlog.0`: ignored
  serial capture from the HTTPS foundation and stack-safety validation. It
  contains the original `sys_evt` overflow and the later successful
  HTTPS/USB/NUT run.
- `build/nut-esp32s3.bin`: ignored local build output; regenerate it from the
  recorded commit rather than treating the artifact as source of truth.
