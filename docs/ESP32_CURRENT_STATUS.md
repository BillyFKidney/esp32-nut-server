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
| Updated | 2026-07-19 17:30 PDT, America/Los_Angeles |
| Active milestone | Operational Management `v2.x` release family |
| Active slice target | ADMIN password management `v2.1.0` |
| Repository branch | `feature/admin-password-management`, created from `main` after PR #10 |
| Validated implementation commit | `0fcd9e1f9` (`fix: start HTTPS management outside system event task`) |
| Remote state | `feature/admin-password-management`, local `main`, and `origin/main` began aligned at `7a653779d`; no publication has occurred |
| Source worktree | Modified ADMIN-management source, `.gitignore`, current project instructions/docs, and reusable `docs/for_next_project/` guidance; generated outputs and macOS `.DS_Store` files are ignored. Untracked `docs/for_next_project/Billy's Initial Project Files.zip` is treated as user-owned and untouched |
| Build environment | ESP-IDF v6.0.2, target `esp32s3` |
| Latest local build | The uncommitted `7a653779d`-based ADMIN password-management worktree builds successfully with ESP-IDF v6.0.2; latest image size is `0x1329f0` bytes with 62% of the smallest application partition free |
| Board | YD-ESP32-23 with ESP32-S3-WROOM-1-N16R8 |
| UPS | CyberPower CST150UC2 on the ESP32 native USB host port |
| Last verified IPv4 address | `192.168.40.173` on 2026-07-19; verify with UniFi at the start of a new session |
| Last observed development USB path | Normal COM rediscovered as `/dev/cu.usbmodem54E20396741`; native USB ROM download used `/dev/cu.usbmodem1101` for recovery and installation |
| Physical intervention required | None; normal UPS connectivity is restored and no RESET is required |

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
- Merged the independently validated HTTPS foundation through PR #10 and split
  the remaining Operational Management scope into small implementation branches.

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

## Current branch work

**Observed on 2026-07-15:** the live device at `192.168.40.173` responded on
HTTPS TCP 443, returned the one-time ADMIN setup page, rejected unauthenticated
status access with HTTP 401, and reported `ups.status = OL` through the
read-only NUT service on TCP 3493. TCP 8080 refused connections. The rediscovered
COM device was `/dev/cu.usbmodem54E20396741`, with no serial-monitor owner; the
serial port was not opened.

The uncommitted branch implementation now adds a CSRF-protected ADMIN password
change route and browser form, one-time setup CSRF protection, exact session
cookie parsing, true server-side idle expiration, session rotation after a
password change, and deterministic HTTP 429 login cooldown responses. The
ESP-IDF v6.0.2 build completed successfully. These changes are **not yet
installed or hardware-tested**.

**Observed later on 2026-07-15:** the authorized COM flash connected to the
correct ESP32-S3 and MAC address, but the esptool RAM-stub upload failed with a
checksum error at both 460800 and 115200 baud before writing flash. A reviewed
no-stub recovery attempt erased the partition-table sector, then failed on its
first write packet because the serial message was corrupted. Serial boot output
now repeats `partition 0 invalid magic number 0xffff` and `Failed to verify
partition table`; HTTPS and NUT are offline. The application and NVS partitions
were not targeted by the failed write, but the board cannot boot until the
4-KB partition-table sector is restored. The single diagnostic monitor was
closed with Ctrl-] and `lsof` confirmed that the COM port was released.
Disconnecting and reconnecting the COM cable did not change the abnormal red
and green LED state or clear the serial corruption; a partition-table-only
write again failed on its first payload packet with ROM result `0105`.

The Device Operator moved the Mac connection to native USB, disconnected the
UPS, and entered ROM download mode. Native USB enumerated as
`/dev/cu.usbmodem1101`; the partition table was restored and hash-verified on
the first attempt. After one RESET press, serial showed the previous
`v1.1.0-4-ged37c1f7c-dirty` firmware booting, recovering saved Wi-Fi at
`192.168.40.173`, and starting HTTPS. Network checks then confirmed HTTP 200 on
HTTPS TCP 443 and an accepting read-only NUT service on TCP 3493. The NUT query
returned `ERR DRIVER-NOT-CONNECTED`, as expected while the UPS is physically
disconnected. The monitor was closed; the native USB serial path disappeared
normally when the application switched that connector to USB host mode.

The first full native-USB installation completed with hashes verified and the
new firmware booted. HTTPS TCP 443 and NUT TCP 3493 were reachable, TCP 8080 was
closed, the root page showed first-run ADMIN setup with a CSRF field, and the
status API rejected unauthenticated access with HTTP 401. Live header inspection
caught a corrupted setup `Set-Cookie` value before any ADMIN password was
submitted. The cause was an ESP-IDF response-header buffer whose lifetime ended
before the response send. The buffer lifetime was corrected in the worktree and
the `0x131a30` replacement image built successfully.

The corrected image was then installed through native USB with every written
hash verified. After RESET, HTTPS returned HTTP 200 at `192.168.40.173`; the
first-run setup page carried a valid five-minute setup cookie with Secure,
HttpOnly, and SameSite=Strict attributes plus a distinct CSRF field. The status
API returned HTTP 401 without authentication and TCP 8080 remained closed. NUT
TCP 3493 refused the connection in the current UPS-disconnected cable state, so
NUT recovery is **not yet tested** for this installed image. No ADMIN password
has been submitted.

Normal cabling was restored and `/dev/cu.usbmodem54E20396741` reappeared with
no serial-monitor owner. HTTPS returned the corrected ADMIN setup page, NUT TCP
3493 reported `ups.status = OL`, and TCP 8080 refused connections. The first
HTTPS request after cable restoration accepted TCP but timed out; the single
bounded retry completed normally with HTTP 200, so no reboot or serial
diagnostic was needed.

Manual first-run setup validation then observed that the Show password control
revealed only the ADMIN password field, not its confirmation field. Server-side
mismatched-password rejection passed. A later matching-password submission was
rejected because the setup form had expired; a subsequent read-only request
still returned the first-run setup page, confirming that **no ADMIN password was
set**. The mismatch rejection remains current functionality; only clearer,
field-specific mismatch feedback is deferred as a Milestone 6 UX improvement.

The worktree now uses explicit DOM element lookups so Show password affects both
setup fields. It also replaces the device-global setup token with a five-minute
per-browser double-submit token, preventing another setup-page request from
invalidating the Device Operator's open form. The corrected `0x131bb0` image
builds successfully with ESP-IDF v6.0.2.

The corrected image was installed through the rediscovered native USB path
`/dev/cu.usbmodem1101`; all flash regions passed hash verification. HTTPS did
not return after the flasher's automatic reset, so the Device Operator tapped
RESET once. Network-first validation then rediscovered the board by its MAC at
`192.168.40.173`; HTTPS returned HTTP 200 with the corrected first-run setup
page, a well-formed five-minute per-browser setup cookie, and JavaScript that
explicitly toggles both ADMIN password fields. Matching password storage and
NUT recovery with normal cabling were **not yet tested** at that point.

The Device Operator then confirmed in the browser that Show password reveals
both setup fields, so that correction passed. Matching 16-character passwords
using upper- and lowercase letters, numbers, and the symbols `.`, `-`, and `!`
were nevertheless rejected with the generic mismatch/length response whether
Show password was enabled or disabled. Source inspection found that the PSA
PBKDF2 inputs were passed in an order rejected by ESP-IDF 6.0.2: password and
salt preceded the iteration cost. The matching input therefore reached hashing,
but hashing failed before NVS storage; the handler incorrectly presented that
internal failure as a mismatch. The worktree now follows the documented PSA
order of cost, salt, then password, and reports storage failures separately.
The corrected `0x131ca0` image builds successfully with ESP-IDF v6.0.2; it is
**not yet hardware-validated** at this point.

The `0x131ca0` PBKDF2-order correction was then installed through the
rediscovered native USB path `/dev/cu.usbmodem1101`; every flash region passed
hash verification. HTTPS again required one manual RESET after the flasher's
automatic reset. The subsequent network-first check rediscovered the board by
MAC at `192.168.40.173`, received HTTP 200 over HTTPS, and observed the
first-run setup page, confirming that no ADMIN password is configured. Matching
password storage was **not yet tested** at that point.

The Device Operator restored normal cabling and successfully saved a matching
16-character ADMIN password without disclosing it. The firmware created the
authenticated session, redirected to the administration console, and the
protected status request displayed the expected device, Wi-Fi, ADMIN role, and
read-only NUT metadata. This validates first-run password storage and automatic
sign-in. A network-first follow-up rediscovered the board by MAC at
`192.168.40.173`, discovered the NUT UPS name `cyberpower`, and observed
`ups.status = OL` on TCP 3493; TCP 8080 refused the connection as required.

Browser validation of ADMIN password change then passed: Show passwords
revealed all three fields; an incorrect current password produced the expected
specific rejection; mismatched new passwords produced the expected length and
mismatch rejection; and a correct current password with matching valid new
passwords changed the password successfully and reloaded the authenticated
console. The success response was visible for only about 800 ms. The worktree
now keeps that confirmation visible for three seconds before the required
reload activates the rotated session and CSRF token; this timing adjustment is
included in a successful ESP-IDF v6.0.2 `0x131ca0` build but is **not yet
installed**.

Login validation passed for ordinary incorrect-password rejection, the fifth
failure starting the cooldown, correct-password rejection during cooldown, and
successful correct-password login after two minutes. Reloading `/login` with
GET returned ESP-IDF's method-invalid response because only POST was registered.
The root page continued to show the login form during cooldown rather than a
lockout-only view. The worktree now registers GET `/login` as a redirect to the
root login state and makes both root and POST responses show only a server-based
remaining-seconds countdown with automatic reload at expiry. These corrections
are included in a successful ESP-IDF v6.0.2 `0x132120` build but are **not yet
installed**.

The Device Operator measured 14 to 17 seconds from pressing Sign in until a
response appeared. **Inferred from the implementation and timing:** the delay
is dominated by the intentionally expensive 100,000-round PBKDF2-SHA-256
calculation on the ESP32-S3, not network transport. The worktree now paints a
`Verifying password...` status before submitting, but the password work factor
has not been reduced. Selecting a target-appropriate derivation cost or a more
efficient strategy remains a security/performance decision and is **not yet
tested**.

The Project Maintainer selected a target of approximately two seconds rather
than retaining the 14-to-17-second verification time. The worktree now stores a
versioned ADMIN credential with its PBKDF2 iteration count and uses 12,500
rounds for new hashes. Existing unversioned credentials remain readable as
100,000-round legacy records; the first successful legacy login rewrites the
same password into the versioned format and removes the legacy salt/hash keys.
The `0x132360` image builds successfully with ESP-IDF v6.0.2. Migration,
target-device timing, and the complete login UX corrections are **not yet
installed or hardware-validated** at that point.

The `0x132360` image was installed through the rediscovered native USB path
`/dev/cu.usbmodem1101`, with every flash region hash-verified. As in the prior
native installations, HTTPS required one manual RESET after the flasher's
automatic reset. Network-first validation then rediscovered the board by MAC at
`192.168.40.173`, received the login page over HTTPS, and thereby confirmed
that the existing ADMIN credential survived. GET `/login` now returns HTTP 303
to `/`, and the delivered login page contains the immediate `Verifying
password...` feedback. Password migration, target login timing, countdown UX,
and NUT recovery after normal cabling are **not yet tested**.

The Device Operator observed immediate `Verifying password...` feedback on the
first installed login, while the legacy verification and migration still took
more than 15 seconds. The Project Maintainer accepted the current password
login/change duration and asked that it be documented rather than optimized
further in this slice. The first successful login is **inferred** to have
migrated the legacy credential; subsequent versioned-credential timing was not
measured and is no longer a completion criterion for this branch.

To avoid repeated USB installations while preserving the prohibition on the
retired unauthenticated TCP 8080 service, the worktree now adds an authenticated
firmware file picker to the ADMIN console. Safari supplies its existing secure
session cookie and the page's CSRF token; the Device Operator selects and
confirms a local `.bin`, and the existing HTTPS OTA handler verifies it before
selecting the inactive slot and restarting. The page reports upload/verification
state and reconnects after restart. The larger generated console page is now
heap-backed rather than consuming the HTTPS task stack. This consolidated
`0x1329a0` image builds successfully with ESP-IDF v6.0.2 and is **not yet
installed or browser/hardware-validated** at that point.

The consolidated `0x1329a0` image was then installed through the rediscovered
native USB path `/dev/cu.usbmodem1101`; every flash region passed hash
verification. HTTPS did not return after the flasher's automatic reset, so one
manual RESET is currently required before network validation. The authenticated
OTA UI is **not yet browser-validated**.

After the Device Operator pressed RESET, the network-first check rediscovered
the board by MAC at `192.168.40.173`. HTTPS returned the installed login page,
the existing ADMIN credential remained configured, immediate verification
feedback was present, and GET `/login` returned HTTP 303 to `/`. Serial was not
opened. Normal cabling and authenticated Safari OTA interaction are **not yet
validated** for this installed image.

The Device Operator then restored browser access in Safari and observed the
authenticated Install firmware section. Pressing Choose File opened Safari's
native file-selection dialog, so OTA control rendering and file-picker
interaction passed. Actual upload, confirmation, image verification, slot
transition, restart, and reconnect remain **not yet tested**. The worktree now
adds running and next OTA partition labels to the protected status JSON so the
first browser OTA can prove the slot transition without serial access.

The slot-observable `build/nut-esp32s3.bin` then built successfully with ESP-IDF
v6.0.2: 1,255,920 bytes (`0x1329f0`), SHA-256
`7d4f2d4114a75997dc715ab90f0a10cb2a9e3102e4ba86127729428f0f7e3efa`.
It was **not yet installed** at that point.

On 2026-07-19, the Device Operator selected that exact image in Safari and
approved authenticated installation. The page displayed `Uploading and
verifying firmware...`; the device verified the image, restarted, and Safari
reconnected to the login page. After authentication, protected status reported
firmware `v1.1.0-10-g7a653779d-dirty`, uptime 39 seconds, running slot `app1`,
and next slot `app0`, proving the expected `app0` to `app1` OTA transition.
Network follow-up observed `cyberpower` `ups.status = OL` through read-only NUT
on TCP 3493 and connection refusal on retired TCP 8080. Authenticated browser
OTA of a known-good local image therefore **passed end to end**. Corrupt-image
rejection remains **not yet tested**.

On the installed consolidated image, the Device Operator repeated the complete
lockout UX. After five incorrect passwords, both `/` and GET `/login` displayed
only the lockout page with the correct remaining seconds. Reloading either route
continued from the server's current value rather than resetting it; the page
automatically reloaded at zero, the normal login form returned, and the correct
password opened the ADMIN console. Login throttling, countdown, GET-route
behavior, automatic recovery, and post-cooldown authentication therefore
**passed**.

The Device Operator then left an authenticated ADMIN console idle for longer
than the configured fifteen-minute window. Reloading required authentication,
and the correct password returned to the ADMIN console. Server-side idle
session expiration and subsequent login therefore **passed**.

The Project Maintainer established a repository-wide collaboration rule: every
service retirement requires explicit approval naming that service, and every
change that materially removes Agent access/automation or transfers recurring
Agent work to a human requires prior disclosure of the before/after workflow,
replacement, responsibility transfer, validation, and rollback plus explicit
approval. The rule is now recorded in root `AGENTS.md`, the ESP32 roles and
development plan, and the reusable `docs/for_next_project/` profiles,
questionnaires, templates, and relevant addenda.

## Implemented versus remaining

### Implemented foundation

- Self-signed device HTTPS on TCP port 443
- Initial ADMIN password creation and sign-in
- Browser session, CSRF, and login-throttling foundation
- Initial status JSON endpoint
- Authenticated local OTA installation endpoint
- Authenticated Safari firmware file picker with confirmation and reconnect
- Three-second Wi-Fi reset and fifteen-second management factory reset
- Read-only NUT service on TCP port 3493
- USB HID polling for the validated CyberPower UPS

### Remaining Operational Management work

- Complete browser validation of ADMIN setup, login, logout, CSRF enforcement,
  and CSRF enforcement
- Revalidate the three-second ADMIN password-change success confirmation on the
  installed consolidated image
- Named API-token creation, display, and confirmed deletion
- Full dashboard data, including UPS identity, serial number, status, battery,
  load, runtime, input/output voltage, NUT health, and last update result
- Wi-Fi scan, signal display, credential change, confirmation, and reconnect
- Corrupt OTA image rejection validation
- Remote service controls and live browser diagnostics
- NTP and IANA time-zone configuration
- Full three-second and fifteen-second physical recovery validation
- iPhone and MacBook Air acceptance testing

## Exact next action

Validate physical ADMIN-password recovery, then complete the branch-level
review and acceptance record for the prospective `v2.1.0` release.

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
