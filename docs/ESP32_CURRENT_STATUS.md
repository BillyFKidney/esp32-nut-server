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
| Updated | 2026-07-21 00:25 PDT, America/Los_Angeles |
| Active milestone | Operational Management `v2.x` release family |
| Active slice target | API tokens `v2.3.0` remain final and published; the `v2.4.0` management-dashboard implementation is in progress on `feature/management-dashboard` |
| Repository branch | `feature/management-dashboard` is based directly on synchronized `main` at `75aa270ed`; no push, merge, tag, or release has been performed |
| Validated implementation state | PR #20 merged the API-token implementation at `595e3dcda`; annotated tag `v2.3.0` points to that merge commit |
| Remote state | Live `origin/main` is synchronized, no pull request is open, and the final GitHub `v2.3.0` release is public |
| Source worktree | Modified only in `include/ota.h`, `src/ota.c`, and `src/management.c`; generated ESP-IDF outputs remain ignored |
| Build environment | ESP-IDF v6.0.2, target `esp32s3` |
| Latest local build | Local `v2.4.0` dashboard candidate built successfully with ESP-IDF v6.0.2; 1,294,080 bytes (`0x13bf00`), SHA-256 `81d994d47671842a5a96b3791e810f0f85480f423c398abe6b143a9541542316`, and 61% of the smallest application partition free; not installed |
| Latest published release | Final `v2.3.0`, tagged at PR #20 merge commit `595e3dcda` and published with the exact-tag ESP32-S3 application image and checksum asset: [GitHub release](https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.3.0) |
| Installed firmware | **Observed before this branch:** the Device Operator reports a successful clean `v2.3.0` installation; the device reports uptime 303 seconds, Wi-Fi `192.168.40.173`, and the restored operational configuration. The local `v2.4.0` dashboard candidate is **not installed** |
| Board | YD-ESP32-23 with ESP32-S3-WROOM-1-N16R8 |
| UPS | CyberPower CST150UC2 on the ESP32 native USB host port |
| Last verified IPv4 address | `192.168.40.173`; post-reset MAC rediscovery, ping, HTTPS 200, NUT/UPS `OL`, and retired-port checks passed, in addition to the earlier unauthenticated route boundaries, supported mixed load, and ten-minute Chrome-connected soak |
| Last observed development USB path | Normal COM rediscovered as `/dev/cu.usbmodem54E20396741` with no listed owner. A bounded console open unexpectedly reset the board despite disabled DTR/RTS; do not reopen it as a nominally read-only diagnostic. Native USB ROM download previously used `/dev/cu.usbmodem1101` for corrective installation |
| Physical intervention required | None; normal Mac COM and UPS native-USB cabling is restored and no RESET is required |

## Current objective

The API-token `v2.3.0` slice is complete and published. The `v2.4.0`
management-dashboard implementation is in progress on
`feature/management-dashboard`, based directly on synchronized `main`.
Continue to preserve LAN-only HTTPS, read-only NUT, and the existing ADMIN and
Agent authorization boundaries. UPS access remains read-only. The next action
is proportional static/source validation followed by explicitly authorized
target installation and browser/API checks; publication remains out of scope.

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
- Added four-slot named API tokens with one-time display, verifier-only NVS
  storage, ADMIN-session-only lifecycle management, and scoped Agent OTA.
- Merged the validated API-token slice through PR #20, tagged it as `v2.3.0`,
  and published the exact-tag firmware and checksum assets.

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

## Branch history and implementation evidence

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

The validated implementation and documentation were committed as `c91430f89`,
pushed to `origin/feature/admin-password-management`, and published for review
as draft PR #12 against `main`. The PR remains draft because physical ADMIN
password recovery has not yet been implemented and validated. The branch has
not been merged or published as `v2.1.0`.

With explicit Project Maintainer authorization, the completed PR #10 foundation
was tagged and published as `v2.0.0` on 2026-07-19. The release tag points to
merge commit `257a983b116fc78789892f8c0edb7d6ae52b5e7a`. Its ESP-IDF v6.0.2
application image reports `v2.0.0`, is 1,248,944 bytes (`0x130eb0`), and has
SHA-256 `3055e875d0d8b74d52d4833c6b17bc041afa06dceb08531ed173694679b934b2`.
GitHub hosts both `nut-esp32s3.bin` and `nut-esp32s3.bin.sha256`; the release is
public, final, and neither a draft nor a prerelease.

The reusable `docs/for_next_project/` starter kit now prompts future projects
to choose a versioning model, map milestones and implementation slices to
prospective releases, treat merge/tag/artifact/release publication as separate
state changes, and audit accepted slices for accidental release gaps. The
distributable ZIP was rebuilt from the updated generic files, passed archive
integrity checks, and contains no `.DS_Store` or `__MACOSX` entries.

The complete documentation and workflow update was merged to `main` through PR
#13 at `0252977e6320c0200dd6bcaed561146395ad86a5` and published as `v2.0.1`.
No firmware source or behavior changed from `v2.0.0`; the optional release image
was rebuilt from the exact tag with ESP-IDF v6.0.2 so it reports `v2.0.1`. The
image is 1,248,944 bytes (`0x130eb0`) and has SHA-256
`86f9e19fe1f864dfae57a5737573276de3437ad988bc06585b99c23a994406c9`.
GitHub hosts both the firmware and checksum assets. The release is public,
final, and neither a draft nor a prerelease. Hardware reinstallation was not
required for this documentation-only patch.

**Observed in source on 2026-07-19:** the original recovery implementation
sampled BOOT only once during application startup. Because BOOT is GPIO0, a
BOOT-plus-RESET sequence selects the ESP32-S3 ROM downloader instead of running
that application check. A normal BOOT press after startup was not monitored.
The worktree now runs a small polling task throughout normal application
operation. A three-second hold erases saved Wi-Fi; continuing through fifteen
seconds arms the management factory reset. The task waits for BOOT to be
released before erasing management NVS and restarting, so the restart cannot
sample GPIO0 low and enter the ROM downloader. The fifteen-second scope remains
Wi-Fi, ADMIN and session state, future API credentials, and the self-signed
HTTPS identity; firmware and OTA slots remain intact.

**Observed build and network evidence:** ESP-IDF v6.0.2 built
`build/nut-esp32s3.bin` successfully at 1,256,784 bytes (`0x132d50`), SHA-256
`d0f7d97e2a8cecea0eff97db7f9f753ea8d2f61217a6812114e2ccb99adcf4a8`.
The current device was rediscovered at `192.168.40.173` by its ESP32-NUT HTTPS
login page; TCP 443 and read-only NUT TCP 3493 accepted connections,
`cyberpower` reported `ups.status = OL`, and retired TCP 8080 refused the
connection. The runtime gesture, erasure, restart, fallback AP, Wi-Fi
reprovisioning, regenerated certificate, and one-time ADMIN setup were **not yet
hardware-tested** at build time.

The Device Operator installed that image through authenticated Safari OTA and
observed the page reconnect to the login screen with firmware
`v2.0.1-6-g1883655d0-dirty`. A subsequent network-first check rediscovered its
ESP32-NUT login page at `192.168.40.173`; HTTPS TCP 443 and read-only NUT TCP
3493 accepted connections, `cyberpower` reported `ups.status = OL`, and TCP
8080 refused the connection. Installation and healthy pre-recovery operation
therefore **passed**. The physical gesture and destructive recovery outcome
remain **not yet tested**.

The Device Operator then held runtime BOOT for sixteen seconds and released it
without pressing RESET. The device restarted into its unique fallback AP, the
iPhone captive portal loaded, Wi-Fi credentials were accepted, and the device
rejoined `ClubHouse_IoT` at `192.168.40.173`. Browsing to HTTPS displayed the
one-time ADMIN setup page, proving that the previous ADMIN credential and HTTPS
identity were cleared. The physical gesture, factory erasure, fallback AP,
Wi-Fi reprovisioning, and return to one-time ADMIN setup therefore **passed**.

Saving either a new or the former ADMIN password then returned `Unable to save
the ADMIN password`. A targeted serial capture reproduced the failure and
**observed** `ESP_ERR_NVS_KEY_TOO_LONG` from the management handler. The
versioned key `admin-credential` contains sixteen characters, exceeding the
ESP-IDF NVS limit of fifteen characters plus the null terminator. The worktree
now uses `admin-cred` and compile-time assertions protect every management NVS
namespace/key length. ESP-IDF v6.0.2 builds the correction successfully at
1,256,784 bytes (`0x132d50`), SHA-256
`01c0bc69665d2bc90527b56aced1d1854b999875ded2d27cb4e3207361e9fb55`.
It is **not yet installed or hardware-tested**. The monitor was closed and the
COM port released; its capture is retained under `cleanup/artifacts/serial/`.

The corrected full image was installed through rediscovered native USB path
`/dev/cu.usbmodem1101`; the ESP32-S3 MAC matched `30:30:f9:16:89:a4` and every
written region passed hash verification. The flasher's automatic reset again
left the board in native USB mode, so the Device Operator tapped RESET once.
The setup page subsequently returned at `192.168.40.173`. The Device Operator
entered a private matching ADMIN password, storage succeeded under the corrected
NVS key, and the authenticated ADMIN console loaded automatically with firmware
`v2.0.1-6-g1883655d0-dirty`. Physical ADMIN-password recovery therefore
**passed end to end**.

Normal cabling was restored without RESET. Network-first validation then
observed HTTPS HTTP 200 at `192.168.40.173`, the CyberPower CST150UC2 with
`ups.status = OL` through read-only NUT TCP 3493, and connection refusal on
retired TCP 8080. Normal COM `/dev/cu.usbmodem54E20396741` was present with no
monitor owner. Post-recovery HTTPS, NUT, UPS polling, service boundaries, and
cable restoration therefore **passed**. The Device Operator also observed that
login was much faster after recovery. **Inferred from the stored credential
format:** the new password uses the intended 12,500-round versioned PBKDF2
record rather than the former 100,000-round legacy verifier; exact timing was
not measured and is **not tested**.

With explicit Project Maintainer authorization, draft PR #12 was marked ready
and merged to `main` at `b35a66cb35e77c9b2ec6b3c98a4b809e54d8c6af`.
Annotated tag `v2.1.0` points to that merge commit. An exact-tag ESP-IDF v6.0.2
build reports `v2.1.0`, is 1,256,784 bytes (`0x132d50`), and has SHA-256
`047ae1ce4cf20d0313342b614c1ae10bb5669eac5a0e0867ad3b9ecfc389b2d9`;
its ESP32 image checksum and validation hash are valid. GitHub publishes the
firmware and 82-byte checksum asset in the final, non-prerelease release.

The Device Operator subsequently installed that exact published image through
the authenticated Safari OTA flow. Safari reconnected after the device restart,
and the administration console reported firmware `v2.1.0`. A network-first
follow-up rediscovered the board at `192.168.40.173` through its known MAC
address, received HTTPS HTTP 200, identified the CyberPower CST150UC2, and
observed `ups.status = OL` through read-only NUT TCP 3493. Retired TCP 8080
refused the connection. Normal COM `/dev/cu.usbmodem54E20396741` was present
with no monitor owner. Exact tagged-image installation and the post-install
HTTPS, NUT, UPS, and service-boundary checks therefore **passed**. The running
OTA slot and rollback state were **not tested** after this installation.

**Observed during the 2026-07-19 23:37 PDT API-token preflight:** local `main`,
its `origin/main` tracking ref, and live GitHub `origin/main` all resolved to
`f27ec9d06` with zero recorded divergence and a clean worktree. GitHub reported
no open pull requests and a final, non-prerelease `v2.1.0` release. Local
`feature/api-tokens` was created directly from that synchronized commit and was
not pushed.

Network-first discovery mapped the known ESP32 MAC address to
`192.168.40.173`. TCP 443 and read-only NUT TCP 3493 accepted connections;
retired TCP 8080 refused the connection. The first bounded HTTPS attempt timed
out during TLS setup, while the single bounded retry returned HTTP 200 with the
ESP32-NUT ADMIN sign-in page. Direct read-only NUT protocol requests identified
the CyberPower CST150UC2 and returned `ups.status = OL`. Unauthenticated live
requests to `/api/v1/status` and `/api/v1/ota/install` were rejected with HTTP
401 and 403 respectively. Normal COM `/dev/cu.usbmodem54E20396741` was
rediscovered with no listed owner; serial was not opened.

The exact published `v2.1.0` installation remains **observed from the prior
validated handoff**. The protected live firmware-version field, running OTA
slot, and rollback state were **not tested** during this unauthenticated
preflight.

**Observed in source:** the existing management implementation provides
cryptographic random generation, hexadecimal encoding, constant-time verifier
comparison, PSA PBKDF2 support, management-namespace NVS persistence, secure
browser cookies, CSRF enforcement, no-store responses, and a caller-authenticated
OTA request processor. It has no API-token record, Bearer-header parser, token
scope enforcement, creation/list/deletion routes, or token UI. The HTTPS server
currently registers exactly eight handlers while its configured route capacity
is also eight. Management time synchronization is not implemented, so a trusted
device-generated issue date is **not available** without pulling later time
configuration into this slice.

**Approved sequencing decision on 2026-07-19 at 23:53 PDT:** the Project
Maintainer moved `feature/time-configuration` forward to `v2.2.0` so API-token
issue dates, dashboard data, OTA results, and live diagnostics can share
device-owned UTC and local timestamps. API tokens move to `v2.3.0`; management
dashboard to `v2.4.0`; Wi-Fi management to `v2.5.0`; local OTA management to
`v2.6.0`; and live diagnostics to `v2.7.0`. Physical recovery and combined
acceptance remain `v2.8.0` and `v2.9.0`.

The unimplemented local branch was renamed to `feature/time-configuration` at
the same `f27ec9d06` base. **Before the reorder,** scoped Agent-driven OTA was
planned for `v2.2.0`. **After the reorder,** it is planned for `v2.3.0`; the
existing authenticated Safari OTA path remains available, and the Device
Operator or Project Maintainer will perform the one additional browser-assisted
`v2.2.0` installation. The Project Maintainer explicitly accepted that delayed
replacement and temporary human step. No service was retired, no existing
capability was removed, and the unauthenticated TCP 8080 service remains closed.

**Observed before time-configuration implementation:** the
authenticated status API exposes uptime but not calendar time, and ESP32-NUT
does not initialize SNTP or manage a time zone. ESP-IDF provides SNTP
initialization, `settimeofday()`, and POSIX time-zone support. Translating the
stored IANA name to the corresponding POSIX rule, tracking synchronization
state/source, and avoiding a false 1970 display were not implemented at that
inspection point.

**Observed in the current worktree and build on 2026-07-20:** the new
time-configuration module stores a versioned record in the existing management
NVS namespace, defaults to automatic SNTP through `pool.ntp.org`, and defaults
to `America/Los_Angeles`. It maps the supported IANA selections to POSIX
daylight-saving rules, starts SNTP asynchronously after station Wi-Fi receives
an address, supports an immediate retry, and permits a manually entered local
date/time from 2024 through 2099. Manual time remains available while NTP
retries and is superseded by a successful synchronization. Unknown time is
reported explicitly rather than as 1970, and wall-clock time is not used as an
authorization, token-expiration, or firmware-trust signal.

The existing authenticated status response now includes UTC/local timestamps,
IANA time zone, source, NTP configuration, synchronization, and pending state.
The ADMIN console adds configuration, synchronize-now, and manual-time controls;
all changes use the existing session and CSRF boundary through
`POST /api/v1/admin/time`. Factory reset already erases the time record because
it clears the complete management namespace. Existing ADMIN password, session,
OTA, HTTPS, and read-only NUT behavior is otherwise unchanged.

ESP-IDF v6.0.2 built `build/nut-esp32s3.bin` successfully. The candidate reports
`v2.1.0-4-gf27ec9d06-dirty`, is 1,274,064 bytes (`0x1370d0`), and has SHA-256
`afd668cfa14ad064a09d015ef9b4ae9f7c08268d1a1fd7dbdf86e2bbe7967da9`.
ESP32-S3 image checksum and validation hash verification passed, with 62% of
the smallest application partition free. At build time, browser rendering,
authenticated API actions, NTP reachability, manual time, persistence,
timezone transitions, firmware installation, reboot, HTTPS/NUT health, and
hardware behavior were **not yet tested**.

**Observed after authenticated Safari OTA on 2026-07-20:** the candidate
reconnected at `192.168.40.173` and reported
`v2.1.0-4-gf27ec9d06-dirty`, running slot `app0`, next slot `app1`, connected
Wi-Fi, and ADMIN HTTPS management. The Device Operator selected
`America/Los_Angeles`, enabled NTP with server `192.168.40.10`, and set the
local date/time manually. The protected status response then showed matching
UTC and PDT values, source `manual`, time available, and NTP synchronization
pending. Manual entry, UTC/local conversion, selected-zone reporting, the
authenticated browser controls, installation, and reboot therefore **passed**
this initial check.

**Observed by network-first diagnosis:** `192.168.40.10` returned valid NTP
responses to the development Mac as a stratum-3 server, and `pool.ntp.org`
also returned valid NTP responses. The ESP32 continued to serve HTTPS on TCP
443 and read-only NUT on TCP 3493; the CyberPower CST150UC2 reported
`ups.status = OL`, and retired TCP 8080 refused connections. The serial port
was not opened. At that point, whether UDP/123 replies from either server
reached the ESP32 and whether its SNTP callback completed were **not yet
tested**; a controlled `pool.ntp.org` attempt was selected as the next
discriminator.

The controlled `pool.ntp.org` attempt also remained pending. Network-only
evidence was then insufficient, so the normal COM path was rediscovered as
`/dev/cu.usbmodem54E20396741` with no owner and one bounded ESP-IDF monitor was
opened. Opening the monitor restarted the board. The boot log showed saved
Wi-Fi reconnecting, the persisted `pool.ntp.org` setting loading, SNTP starting
immediately after DHCP, HTTPS starting outside the system event task, the UPS
reconnecting, and read-only NUT becoming active. No SNTP callback followed.
The monitor was closed with Ctrl-] and `lsof` confirmed that the port was
released.

**Observed root cause in ESP-IDF/lwIP and project source:** lwIP's
`sntp_setservername()` retains the initial server-name pointer rather than
copying its contents. The project passed `configuration->ntp_server` from a
caller-local configuration record; the configured startup delay therefore
used an invalid pointer after that caller returned. The worktree now copies the
selected server into module-lifetime storage before initializing SNTP and does
not replace that storage until the prior SNTP instance is deinitialized. This
explains the identical pending behavior for a literal local address and a
public hostname; successful target synchronization after the correction is
**not yet tested** at that inspection point.

ESP-IDF v6.0.2 built the corrected image successfully at 1,274,096 bytes
(`0x1370f0`), SHA-256
`2017f020b328b0776d0bf051859e25c44664ddb321e53166d2be39415f0db44d`.
Its ESP32-S3 checksum and validation hash are valid, and the smallest
application partition remains 62% free. After the monitor-induced restart,
TCP 443 and TCP 3493 accepted connections, the CyberPower CST150UC2 reported
`ups.status = OL`, and TCP 8080 refused the connection.

**Observed after the corrected authenticated Safari OTA installation:** the
protected status response reported the expected dirty candidate version,
running slot `app1`, next slot `app0`, connected Wi-Fi, ADMIN HTTPS management,
and read-only NUT metadata. Without another time-setting change, persisted
`pool.ntp.org` synchronized successfully after boot. Time was available with
source `ntp`, `ntp_synchronized = true`, and
`synchronization_pending = false`; UTC `2026-07-20T08:11:42Z` converted to
local `2026-07-20T01:11:42-0700` under `America/Los_Angeles`. This validates
the pointer-lifetime correction, automatic hostname-based NTP, callback state
transition, persisted server/time-zone loading, UTC/local status formatting,
and the current PDT rule.

A network-first regression check mapped the known ESP32 MAC to
`192.168.40.173`, reached HTTPS TCP 443 and read-only NUT TCP 3493, identified
the CyberPower CST150UC2, and observed `ups.status = OL`. Retired TCP 8080
refused the connection. Serial was not reopened.

The Device Operator then changed only the NTP server to `192.168.40.10` and
saved the configuration. After the bounded wait, protected status reported
source `ntp`, `ntp_synchronized = true`, and
`synchronization_pending = false`, with correct UTC/PDT formatting. The local
direct-IP NTP server path therefore **passed** in addition to the public
hostname path.

The Device Operator then disabled NTP without changing the server, time zone,
or clock. Protected status reported time still available, NTP disabled, and no
synchronization pending. Source `ntp` and `ntp_synchronized = true` remain as
provenance for the current clock's last successful synchronization; they do
not claim that polling remains enabled. NTP shutdown with retained running time
therefore **passed**.

With NTP still disabled, the Device Operator changed only the time zone to
`America/New_York`. UTC continued without a step while local time changed to
EDT with the correct `-0400` offset. Alternate-zone application independent of
NTP activity therefore **passed**.

The Device Operator restored `America/Los_Angeles`; UTC again continued
without a step and local time returned to PDT with the correct `-0700` offset.
Protected status still reported `ntp_enabled = false`, so restoration of the
intended automatic-sync state is **not yet complete**. Source inspection shows
the browser explicitly submits the checkbox state as `true` or `false` and the
server persists that submitted value; a checked-and-saved retry is the next
discriminator between an omitted UI action and a runtime defect.

The checked-and-saved retry reported NTP enabled, source `ntp`, synchronized,
and not pending with `America/Los_Angeles` and `192.168.40.10`. The intended
steady-state configuration is restored; the prior disabled result reflected
the submitted checkbox state rather than a persistence defect.

Entering `bad server` produced the expected validation response, `Use a valid
NTP hostname and supported IANA time zone.` Reloading status showed the saved
`192.168.40.10` value unchanged, NTP enabled and synchronized, and no pending
operation. Invalid-hostname rejection and non-mutation therefore **passed**.
An unauthenticated network request to `POST /api/v1/admin/time` with a benign
sync action returned HTTP 403 and `Invalid session or CSRF token.` The time
route's unauthenticated boundary therefore **passed** without serial access or
state mutation.

The Device Operator clicked **Synchronize now** with the restored steady-state
configuration. The browser reported `NTP synchronization requested.` and the
bounded follow-up status showed source `ntp`, NTP enabled and synchronized,
and no pending operation. The explicit retry control and API action therefore
**passed**.

The final network-first regression check mapped the known ESP32 MAC to
`192.168.40.173`, reached HTTPS TCP 443 and read-only NUT TCP 3493, and observed
connection refusal on retired TCP 8080. Correctly CRLF-framed NUT requests
identified the CyberPower CST150UC2 and returned `ups.status = OL`. Serial was
not reopened. The final source audit found no TCP 8080 restoration or UPS
control additions, the installed image SHA-256 still matched the validated
build, and local implementation commit `e4430b81e` was created without pushing.

**Observed during authorized publication on 2026-07-20:** PR #16 merged the
validated time-configuration branch to `main` at
`7cfa26f8a78c8f3c6d9561db5b9af646e64a59f1`. Annotated tag `v2.2.0` points to
that merge commit. The exact-tag ESP-IDF v6.0.2 build reports `v2.2.0`, is
1,274,096 bytes (`0x1370f0`), and has SHA-256
`0a67815bd32581e9c2f89174a12629d2a00e5b2aadb2e83b4cf977eb0d6e3b7e`.
Its ESP32 checksum and validation hash are valid, and 62% of the smallest
application partition remains free. GitHub publishes that exact application
image and its 82-byte checksum asset in the final, non-prerelease `v2.2.0`
release. GitHub's stored firmware digest matches the local exact-tag build.
At publication time, installation and post-install validation of the exact
published image were **not yet tested**; the following observation closes that
gap.

**Observed after authenticated Safari OTA installation of the exact published
image:** the administration status reported firmware `v2.2.0`, running slot
`app0`, next slot `app1`, connected Wi-Fi, ADMIN HTTPS management, and
read-only NUT metadata. The persisted `America/Los_Angeles` time zone and
`192.168.40.10` NTP server loaded without reconfiguration; time was available
with source `ntp`, synchronization true, and no pending operation. UTC
`2026-07-20T09:18:09Z` converted correctly to PDT
`2026-07-20T02:18:09-0700`.

Network-first discovery independently mapped ESP32 MAC
`30:30:f9:16:89:a4` to `192.168.40.173`. The HTTPS management page returned
HTTP 200, read-only NUT TCP 3493 enumerated `cyberpower` as the CyberPower
CST150UC2 and returned `ups.status = OL`, and retired TCP 8080 refused the
connection. Normal COM `/dev/cu.usbmodem54E20396741` was rediscovered with no
listed owner; network evidence was sufficient, so serial was not opened. Exact
published-image installation, persisted time configuration, NTP
synchronization, OTA-slot reporting, HTTPS, NUT/UPS operation, and service
boundaries therefore **passed**. No physical intervention was required.

**Observed during the 2026-07-20 API-token preflight:** local `main`, its
tracking ref, and live GitHub `origin/main` all resolved to `f15989464` with a
clean worktree and zero recorded divergence. GitHub reported no open pull
requests. Network-first discovery mapped MAC `30:30:f9:16:89:a4` to
`192.168.40.173`; ping, HTTPS TCP 443, and read-only NUT TCP 3493 responded.
HTTPS returned HTTP 200, direct NUT protocol requests identified the CyberPower
CST150UC2 and returned `ups.status = OL`, and retired TCP 8080 refused the
connection. Normal COM `/dev/cu.usbmodem54E20396741` was present with no listed
owner; serial was not opened. Local `feature/api-tokens` was then created
directly from the synchronized commit and was not pushed.

**Observed in the local API-token implementation and build:** the branch adds a
versioned four-record token store under the existing management NVS namespace.
Each complete token contains a fixed version prefix and 256 device-generated
random bits. NVS retains only its unique name, random public identifier,
device-generated UTC issue time, final four characters, `ota.install` scope,
random 16-byte salt, and salted SHA-256 verifier. Authorization derives and
compares all four verifier slots without early success. Active names are unique
without regard to ASCII letter case. The complete token exists only in the
creation response and is zeroized after that response is sent.

ADMIN-session-only `GET`, CSRF-protected `POST`, and CSRF-plus-acknowledgement
`DELETE` handlers share `/api/v1/admin/tokens`. The separate
`POST /api/v1/agent/ota/install` route accepts only a strict Bearer
Authorization header with `ota.install`; it reuses the existing verified OTA
request processor. Existing `/api/v1/ota/install` remains ADMIN-session and
CSRF protected for Safari. API tokens cannot authenticate token management,
status, password, time, logout, browser, or other management routes. HTTPS
handler capacity increased from the former exact 9-of-9 state to 16 for 13
registered handlers.

The ADMIN console now lists name, UTC issue date, scope, and final four; displays
the full token in a one-time warning only after creation; and uses a deletion
dialog whose explicit delete button remains disabled until the acknowledgement
checkbox is selected. The Agent OTA helper reads the token only from a private
environment variable, never accepts it in process arguments, does not print the
Authorization header, and requires a trusted certificate, pinned SHA-256
fingerprint, or an explicit unsafe diagnostic override. Existing Safari OTA,
LAN-only HTTPS TCP 443, read-only NUT TCP 3493, and retired TCP 8080 behavior
are unchanged in source.

ESP-IDF v6.0.2 built `build/nut-esp32s3.bin` successfully. The candidate reports
`v2.2.0-6-gf15989464-dirty`, is 1,285,792 bytes (`0x139ea0`), and has SHA-256
`5cffd4d5b04a8f7e2eee608439cd66aa4e8f1f2b42a82b96cc6356fc831f958f`.
Its ESP32-S3 checksum and validation hash are valid, and the smallest
application partition remains 62% free. The generated administration page is
12,453 bytes within its 14,000-byte heap buffer, and its exact generated
JavaScript parses successfully. Later rendered Chrome evidence is recorded
below separately from this static check.

**Observed after authenticated browser OTA:** the Device Operator reported
`Firmware verified. Restarting now.` and the reconnected ADMIN status reported
the expected `v2.2.0-6-gf15989464-dirty` identifier from `app1`, with `app0`
next. Synchronized NTP/local time configuration persisted, the API-token UI
rendered with no active tokens, and NUT remained declared read-only. The exact
installed SHA-256 is **inferred** from the recorded artifact selected for the
upload; the device does not expose an independent running-image digest.

**Observed independently over the network after installation:** MAC
`30:30:f9:16:89:a4` rediscovered at `192.168.40.173`; ping and TCP 443/3493
responded, TCP 8080 refused the connection, HTTPS returned HTTP 200, and NUT
reported the CyberPower CST150UC2 with `ups.status = OL`. Token metadata without
an ADMIN session returned 401. Agent OTA with no Authorization header, an
invalid Bearer value, a query-only credential, or a cookie-only credential also
returned 401. The device certificate SHA-256 fingerprint was observed as
`32:13:A1:E1:67:69:BA:4C:AD:EF:F1:69:24:97:2F:88:2C:9F:33:0D:C5:AC:37:AD:FC:37:A5:51:12:55:EF:C5`.
Normal COM `/dev/cu.usbmodem54E20396741` remained present with no listed owner;
serial was not opened.

**Observed during Device Operator token UI testing:** invalid-name rejection,
first through fourth token creation, versioned token format, issue-date/final-
four metadata, absence of secret fields from the list, exactly-once display
across reload, no-store behavior, case-insensitive duplicate rejection,
four-token capacity enforcement, stable UI/NUT operation at capacity, deletion
dialog acknowledgement gating and cancellation, confirmed deletion, and
capacity recovery all passed. Two independently retained active tokens each
authenticated far enough to reach the Agent route's HTTP 415 media-type gate.
Four active tokens are present. No complete token was included in the test
report or screenshot.

**Observed during Codex Chrome and stability testing on 2026-07-20:** Chrome
successfully reused the authenticated ADMIN session and rendered all four token
metadata records without exposing a complete token. The delete dialog required
its acknowledgement checkbox, kept its final action disabled before
acknowledgement, enabled it afterward, and Cancel preserved all four records.
At a 375-pixel content width the page, controls, and delete dialog had no
horizontal overflow; Chrome reported no console errors or warnings. The
normal viewport was restored afterward.

New non-Chrome HTTPS handshakes and NUT requests then began accepting TCP and
immediately resetting. Network-first evidence was insufficient, so the
rediscovered, unowned COM port was opened for a bounded diagnostic with DTR and
RTS disabled. Opening it nevertheless caused an unexpected power-on reset. No
flash write occurred. The boot log confirmed the candidate still booted from
`app1`, restored saved Wi-Fi at `192.168.40.173`, started HTTPS on TCP 443,
synchronized through the configured NTP server, and rediscovered the UPS.

Immediately after that reboot, HTTPS returned 200, NUT returned
`ups.status = OL`, TCP 8080 refused the connection, unauthenticated ADMIN token
metadata returned 401, and missing, malformed, query-only, and cookie-only
Agent OTA credentials returned 401 with the expected Bearer challenge. Chrome
reload then correctly invalidated the old in-memory ADMIN session and displayed
the sign-in page. The first 30-second soak sample after that Chrome load again
found both new HTTPS and NUT application connections reset, so the soak failed
and was stopped. **Inferred from configuration, not yet proven by an on-target
socket census:** persistent HTTPS client use, the HTTPS listener and control
socket, NUT sockets, and the global `CONFIG_LWIP_MAX_SOCKETS=10` limit are
exhausting the shared socket table despite HTTPS LRU purge being enabled. The
HTTPS server can retain up to four open client sockets. **Not yet tested:**
the exact socket owner/count at failure, whether increasing the global socket
limit or reducing HTTPS client capacity is the correct fix.

The tracked correction sets `CONFIG_LWIP_MAX_SOCKETS=16` while retaining the
four-client HTTPS capacity. ESP-IDF v6.0.2 rebuilt the complete candidate and
confirmed the generated configuration uses 16 sockets. The resulting
`build/nut-esp32s3.bin` remains 1,285,792 bytes (`0x139ea0`) with SHA-256
`c12aee62fd269b611e8105c331b2ae98953e7d2325d7d45d9ffc887d111fe1b9` and 62%
of the smallest application partition free. A fresh Chrome ADMIN login also
confirmed that all four token metadata records persisted across the unexpected
reboot without displaying a complete token. The authorized Chrome OTA attempt
reached the file chooser, but Chrome rejected attaching the local image because
the ChatGPT Chrome Extension does not currently have file-URL access. No upload
or device write occurred.

**Observed after enabling the approved Chrome file permission:** Chrome attached
the exact 1,285,792-byte `c12aee62fd269b611e8105c331b2ae98953e7d2325d7d45d9ffc887d111fe1b9`
candidate, the authenticated OTA confirmation was accepted, the upload caused
the expected temporary network outage and restart, and HTTPS recovered. The
authenticated console then reported `app0` running with `app1` next and all four
token metadata records intact. The full token values were not displayed. The
pre-reboot ADMIN session was invalid after an explicit reload, and a normal
1Password-assisted ADMIN login succeeded without disclosing the password.

With Chrome connected, two simultaneous new HTTPS requests and two simultaneous
NUT requests all passed. A deliberate over-capacity probe of four simultaneous
new HTTPS requests plus four NUT requests produced three HTTPS 200 responses and
one reset; all four NUT requests returned `ups.status = OL`, and the immediately
following HTTPS request returned 200. This is the configured four-client HTTPS
ceiling under an already connected browser, not recurrence of the former global
socket-table exhaustion. A ten-minute soak then produced 20 consecutive HTTPS
200 and NUT `ups.status = OL` samples at 30-second intervals while Chrome
remained connected. Afterward, HTTPS still returned 200, TCP 8080 refused the
connection, unauthenticated ADMIN token metadata returned 401, and missing,
malformed, query-only, and cookie-only Agent OTA credentials each returned 401.

Server-side deletion acknowledgement enforcement, the remaining ADMIN/CSRF
cross-boundaries, rollback state after Agent OTA, Safari rendering of the new UI,
factory-reset erasure, and the full post-Agent-OTA regression sequence are
**not yet tested**. Valid-token Agent OTA, token-record reboot persistence,
immediate revocation, browser OTA, and a passing Chrome-connected target soak
are now observed. A fingerprint-pinned
`tools/esp32-api-token-probe.py`
diagnostic client is present for small authorization-boundary requests without
placing the token in process arguments or output.

**Observed on 2026-07-20 20:32 PDT:** a Device Operator run of the Agent OTA
helper with a privately supplied, format-valid token ended with a client-side
`Broken pipe` before an HTTP response was received. The device remained
reachable afterward: HTTPS returned 200 and NUT returned `ups.status = OL`.
This does not establish whether the token was inactive or the upload transport
closed early; a zero-byte, fingerprint-pinned authorization probe is the next
diagnostic and must return 413 before another full upload is attempted.

**Observed on 2026-07-20 20:35 PDT:** the zero-byte probe returned HTTP 413,
which proves the supplied token passed Bearer authorization and reached the OTA
handler. The shared OTA processor now retries three receive timeouts before
aborting, consistent with the ESP-IDF HTTP-server receive contract. ESP-IDF
v6.0.2 rebuilt the candidate successfully at 1,285,920 bytes with SHA-256
`13ba20f81c52721c8214d7bc9aa280fedc4c9ec009634582b34191d053b6cb5f`; it has
not yet been installed on the target.

**Observed on 2026-07-20 20:50 PDT:** after the corrected candidate was
installed through the browser workflow, the fingerprint-pinned Agent OTA helper
with a valid retained token returned HTTP 200 and
`{"status":"installed","message":"Firmware verified. Restarting now."}`.
The device recovered afterward with HTTPS 200, read-only NUT
`ups.status = OL`, and TCP 8080 refused. ADMIN token metadata without a browser
session returned 401; missing, malformed, query-only, and cookie-only Agent
credentials each returned 401. The post-Agent-OTA running/next slot and token
metadata persistence were then confirmed in the authenticated console: `app1`
is running, `app0` is next, and all four token metadata records remain present.

**Observed on 2026-07-20 20:53 PDT:** the token used for the successful Agent
OTA was deleted through the acknowledged ADMIN dialog. Reusing that retained
token with the fingerprint-pinned zero-byte Agent probe returned HTTP 401
Unauthorized with the expected Bearer-scope error. This confirms immediate
revocation; three active token metadata records remain after the deliberate
deletion.

**Observed on 2026-07-20 21:04 PDT:** a post-Agent-OTA ten-minute soak ran 20
consecutive HTTPS and read-only NUT samples at 30-second intervals. Every
sample returned HTTPS 200 and `VAR cyberpower ups.status "OL"`; no reboot,
socket exhaustion, or serial intervention occurred.

**Observed on 2026-07-20 21:10 PDT:** the second retained active token,
`Agent OTA2`, completed another full Agent OTA installation with HTTP 200 and
the verified-restart response. Its post-reboot zero-byte probe returned HTTP
413, confirming that this second token remains authorized after reboot. HTTPS
returned 200, NUT remained `ups.status = OL`, and TCP 8080 remained refused.

The authenticated console then confirmed the expected alternating-slot state:
`app0` is running and `app1` is next.

**Observed on 2026-07-20 21:15 PDT:** Chrome UI regression on the corrected
firmware passed with the authenticated administration console. The three active
token metadata rows rendered without any complete token in the DOM or screenshot;
the normal viewport rendered correctly; and a temporary 375-pixel viewport had
document and body widths within the viewport with no horizontal overflow. The
delete dialog exposed the acknowledgement checkbox, kept its final action
disabled before acknowledgement, and Cancel preserved the selected token. No
Chrome console warnings or errors were captured. The temporary viewport was
reset before handoff.

**Observed on 2026-07-20 22:28 PDT:** the Device Operator completed the
destructive fifteen-second BOOT factory reset without pressing RESET, restored
Wi-Fi and the ADMIN password, and observed the API-token page with no active
tokens. A previously valid token was then rejected by the Agent OTA endpoint
with HTTP 401 and the expected `ota.install`-scope error. This completes the
factory-reset and post-reset token-erasure checks without recording any
credential or token value.

**Observed on 2026-07-20 22:38–22:40 PDT:** the delayed browser OTA upload
completed and the authenticated console reported firmware `2.3.0`, uptime 18
seconds at first observation, `app1` running with `app0` next, synchronized
NTP, and an empty API-token list. Network-first follow-up rediscovered MAC
`30:30:f9:16:89:a4` at `192.168.40.173`; ping and HTTPS returned, HTTPS returned
HTTP 200, read-only NUT identified the CyberPower CST150UC2 and returned
`ups.status = OL`, and TCP 8080 refused the connection. The exact installed
image digest is **inferred** from the previously built no-prefix `2.3.0`
candidate (`5cb0624cb1d89885477e029987c22246042db6c9d215dbc4fe5c909a9a1d87da`);
the later local `v2.3.0`-prefixed candidate was not installed.

**Observed from the Device Operator on 2026-07-20 22:54 PDT:** a subsequent
clean `v2.3.0` firmware installation completed successfully. The reported
device status included uptime 303 seconds, Wi-Fi IP `192.168.40.173`, and the
restored operational configuration. The exact installed image digest and
latest OTA-slot state were not independently exposed for this report.

**Observed during authorized publication on 2026-07-20 22:59 PDT:** PR #20
merged the API-token slice to `main` at
`595e3dcda24b7fb894ca2e2ea3ca82dc08361a42`. Annotated tag `v2.3.0` points to
that merge commit, and the final GitHub release published `nut-esp32s3.bin`
with SHA-256
`1bf58d525715af4d5f8f00c535ba07fe174a61af9b5dbe2c2d57a69bf2686357` plus its
checksum asset. Local `main` is clean and synchronized with `origin/main`; the
release is not a draft or prerelease. The post-release documentation sync is
recorded on `main`.

**Observed during the v2.4.0 preflight on 2026-07-20 23:21 PDT:** local
`main` was clean at `2c955b6496fd5becc3c4cc8d5842c28992b0a82c` and matched
`origin/main`; the final `v2.3.0` release was present and no pull request was
open. Network-first rediscovery found MAC `30:30:f9:16:89:a4` at
`192.168.40.173` on `en0`; ping succeeded, HTTPS returned HTTP 200, TCP 443
and TCP 3493 accepted connections, and retired TCP 8080 refused the
connection. The unauthenticated status and token-management routes returned
HTTP 401. A CRLF NUT probe identified `CyberPower CST150UC2` and returned
`ups.status = OL`. The normal development USB path was rediscovered as
`/dev/cu.usbmodem54E20396741` with no listed owner; serial was not opened.
The Mac environment did not provide `upsc`, so the bounded raw read-only NUT
protocol was used instead.

**Inferred:** the Device Operator's latest report indicates that clean
`v2.3.0` remains installed with restored Wi-Fi and ADMIN configuration, but
the protected status JSON and current OTA slots were not independently checked
in this preflight.

**Observed on 2026-07-21 00:25 PDT:** after the required preflight, the
`feature/management-dashboard` branch was created directly from synchronized
`main` at `75aa270ed`. The existing authenticated `/api/v1/status` route now
builds an escaped, no-store JSON snapshot containing firmware, uptime, Wi-Fi,
synchronized time, OTA result, NUT health, UPS identity/serial, status,
battery, load, runtime, voltage, and power fields. The authenticated console
now renders those values in dashboard cards while retaining the existing ADMIN
session, CSRF, time, token, and OTA controls. The existing 13 HTTPS handlers
remain within the configured capacity of 16. The ESP-IDF v6.0.2 build passed;
the candidate image is 1,294,064 bytes with SHA-256
`81d994d47671842a5a96b3791e810f0f85480f423c398abe6b143a9541542316` and has
61% of the smallest application partition free.

**Inferred:** dashboard UPS values are read from the existing NUT dstate
fields populated by the USB HID driver; the HTTPS task reads a bounded
snapshot and reports a separate stale/unavailable health state. OTA result
metadata is non-secret and stored in the existing management NVS namespace;
factory reset erases it with the other management data.

**Not tested:** the local `v2.4.0` candidate has not been installed. Protected
status JSON values, dashboard rendering, browser layout, OTA-result transitions
after a real install, NUT stale/unavailable behavior, reboot persistence, and
target-hardware regression checks remain pending. Serial remains unopened.

## Implemented versus remaining

### Implemented foundation

- Self-signed device HTTPS on TCP port 443
- Initial ADMIN password creation and sign-in
- Browser session, CSRF, and login-throttling foundation
- Initial status JSON endpoint
- Authenticated device time, NTP, and supported IANA time-zone configuration
- Authenticated local OTA installation endpoint
- Named, non-expiring API tokens with ADMIN-only lifecycle management
- Scoped Bearer-authenticated Agent OTA installation endpoint
- Authenticated Safari firmware file picker with confirmation and reconnect
- Runtime-monitored three-second Wi-Fi reset and fifteen-second management
  factory reset; the fifteen-second ADMIN recovery path passed end to end
- Read-only NUT service on TCP port 3493
- USB HID polling for the validated CyberPower UPS

### Remaining Operational Management work

- Install and validate the `feature/management-dashboard` candidate on the
  target through an explicitly authorized OTA or USB workflow, then exercise
  the authenticated dashboard and status API.
- Wi-Fi scan, signal display, credential change, confirmation, and reconnect
- Corrupt OTA image rejection validation
- Remote service controls and live browser diagnostics
- Standalone three-second Wi-Fi-only recovery validation in the later physical
  recovery slice
- iPhone and MacBook Air acceptance testing

## Exact next action

Finish static review and target-independent checks for the local
`feature/management-dashboard` build. Before installing it, obtain explicit
authorization for the target update; then validate the dashboard/status API,
NUT health and UPS fields, existing ADMIN/CSRF behavior, OTA result reporting,
and network service continuity. Do not modify the published `v2.3.0` release
or publish this branch.

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
- `cleanup/artifacts/serial/2026-07-19-admin-recovery-password-storage.txt`:
  ignored targeted capture containing the decisive
  `ESP_ERR_NVS_KEY_TOO_LONG` failure after physical ADMIN recovery.
- `build/nut-esp32s3.bin`: ignored local build output; regenerate it from the
  recorded commit rather than treating the artifact as source of truth.
