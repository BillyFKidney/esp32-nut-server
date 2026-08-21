# ESP32-NUT active development plan

This is the authoritative forward roadmap. Completed v2.7.1 scope and released
refactoring details are preserved in [archive/](archive/README.md). The
archived plan snapshot is historical context only; future roadmap content is
maintained here.

## Version and publication rule

Each release is one independently reviewable, validated slice. A merge does
not publish a version: the Project Maintainer separately authorizes the tag,
release assets, and any target installation. Do not consume a later version
for completed maintenance work without updating this table first.

## Release Execution

After the release slice is merged, the Codex Agent executes this workflow. It
must stop before the release tag in step 3 until the Project Maintainer
explicitly says, **“Tag vX.Y.Z and publish release.”** A build, upload, or
target installation remains a separate authorization boundary.

1. Create a release-evidence index and an unpushed evidence tag that identify
   the merged commit, validation results, and intended firmware artifact.
2. Await the Maintainer's explicit `Tag vX.Y.Z and publish release`
   authorization.
3. Create the annotated release tag `vX.Y.Z` with a link to the
   release-evidence index.
4. Build the firmware from that tagged commit for the intended target.
5. Generate and verify the SHA-256 checksum against the exact
   `build/esp32-nut-server.bin` artifact.
6. Publish the GitHub release with `build/esp32-nut-server.bin` and
   `build/esp32-nut-server.bin.sha256`, and link the release evidence.
7. Update [ESP32_CURRENT_STATUS.md](ESP32_CURRENT_STATUS.md) with the
   publication, validation state, and next action.

## Published baseline

`v2.7.2` is published and target-tested. It invalidates stale external UPS
data across physical HID loss, driver staleness, and bounded diagnostics while
keeping OTA and diagnostics credentials isolated. `v2.7.1` completed the
management route families, credential-promotion safety repair, and browser-log
handshake filter without changing the protected service, authorization, Wi-Fi,
or read-only UPS boundaries.

## Operational Management completion — `v2.x`

The locked requirements remain in
[ESP32_DEVELOPMENT_MILESTONE_QA_OPERATIONAL_MANAGEMENT.md](ESP32_DEVELOPMENT_MILESTONE_QA_OPERATIONAL_MANAGEMENT.md).
The ADMIN console, LAN-only HTTPS, read-only NUT access, and physical-recovery
boundaries remain in force. The remaining umbrella-milestone slices are:

| Release | Prospective branch | Required outcome |
| --- | --- | --- |
| `v2.8.0` | `feature/physical-recovery` | Complete and validate the three-second Wi-Fi reset and fifteen-second factory-reset behavior and scope. |
| `v2.9.0` | `feature/operational-management-acceptance` | Validate the locked definition of done from iPhone and MacBook Air and publish the final `v2.x` acceptance release. |

## UPS state, identity, and compatibility — `v2.7.2`–`v2.7.9`

The following observations remain active implementation evidence: disconnecting
the CyberPower UPS can leave old identity/values visible while NUT is stale; a
factory reset with the UPS absent can retain displayed state; and an APC
BR1500G previously caused freeze/reboot symptoms after healthy communication
through the Mac mini. Root causes remain to be established per slice.

| Release | Prospective branch | Required outcome |
| --- | --- | --- |
| `v2.7.3` | Released | Uses monotonic dstate timing to purge external UPS values after five minutes stale. A full successful poll alone resets timing; Agent and physical acceptance preserve immediate stale protection. |
| `v2.7.4` | Released | [APC Back-UPS RS 1500G compatibility](archive/v2.7.4/ESP32_V2_7_4_APC_BR1500G_SPEC.md): from normal post-factory-reset configuration, it communicates without freeze/reboot and reports only validated identity, status, and available measurements. |
| `v2.7.5` | Released | [NUT compatibility hardening evidence](archive/v2.7.5/evidence.md): bounded USB HID/NUT parsing, allocation cleanup, metadata termination, and validated CyberPower/APC behavior; unsupported or malformed hardware remains unclaimed. |
| `v2.7.6` | Released | [Replacement-UPS reprobe evidence](archive/v2.7.6/evidence.md): attachment-generation invalidation, driver-task-only reprobe, and full-poll-gated APC/CyberPower replacement recovery. |
| `v2.7.7` | Released | [Factory-reset evidence](archive/v2.7.7/evidence.md): release-confirmed reset of all defined user values, including UPS identity/cache state, while preserving firmware and documented recovery boundaries. |
| `v2.7.8` | Released | [Status UI evidence](archive/v2.7.8/evidence.md): `nut.ups` contract rename, physical manufacturer/model dashboard presentation, expanded raw status, and verified v2.7.7 rollback/v2.7.8 restore. |
| `v2.7.9` | `main` (merged; tag pending) | [Device identity and log level](ESP32_V2_7_9_DEVICE_CONFIGURATION_SPEC.md): configurable `device_name`, safe derived hostname, reboot-persistent log-level dropdown, and the validated status-response stack-pressure repair. |
| `v2.7.10` | `feature/status-ui-polish` | Browser-only status presentation polish, beginning with an explicit friendly-label mapping for the NUT CyberPower fallback `CPS` to `CyberPower Systems`. Preserve raw API values, stale/unavailable handling, NUT service identity, and all authorization/service boundaries. |

Factory-reset state clearing remains the final persisted-state-clearing slice;
the following identity and presentation slices do not expand its reset scope.

## Production OTA — `v3.x`

- Build on the authenticated HTTPS local-upload route; do not restore an
  unauthenticated development listener.
- Replace the self-signed management certificate with reviewed local-CA trust.
- Verify signed firmware metadata or an ESP-IDF-supported signed-image and
  secure-boot strategy before unattended updates.
- Define a controlled release-asset/version-manifest workflow. Manual check,
  download, and install come before any opt-in schedule; automatic install
  remains disabled by default.

| Release | Prospective branch | Scope |
| --- | --- | --- |
| `v3.0.0` | `feature/local-ca-trust` | Local-CA trust and provisioning model. |
| `v3.1.0` | `feature/signed-update-metadata` | Signed release metadata or supported signed-image strategy. |
| `v3.2.0` | `feature/remote-update-client` | Certificate-validated remote check/download with manual approval. |
| `v3.3.0` | `feature/scheduled-updates` | Opt-in check scheduling; automatic installation remains disabled. |
| `v3.4.0` | `feature/production-ota-acceptance` | Validate authorization, source resistance, rollback, recovery, and definition of done. |

## NUT and UPS compatibility hardening — `v4.x`

Test additional CyberPower and USB HID UPS models, improve evidenced
descriptor/driver selection, and validate read-only NUT interoperability with
`upsc`, Home Assistant/NUT clients, and monitoring systems. UPS writes remain
blocked pending a separately reviewed control milestone.

| Release | Prospective branch | Scope |
| --- | --- | --- |
| `v4.0.0` | `feature/nut-client-interoperability` | Validate representative read-only NUT clients. |
| `v4.1.0` | `feature/cyberpower-compatibility` | Test additional available CyberPower devices. |
| `v4.2.0` | `feature/usb-hid-compatibility` | Improve bounded diagnostics and driver selection for evidenced gaps. |
| `v4.3.0` | `feature/nut-ups-acceptance` | Publish supported-device/client matrix and sustained-operation evidence. |

## Platform resilience and release automation — `v5.x`

Decide whether to expand the lower-8-MB layout while preserving dual OTA;
add exact-target builds and release provenance; document upgrade/recovery; and
evaluate secure boot, flash encryption, and certificate storage as separate
risk boundaries.

| Release | Prospective branch | Scope |
| --- | --- | --- |
| `v5.0.0` | `feature/flash-layout` | Validate any storage-layout expansion and dual-OTA recovery. |
| `v5.1.0` | `feature/release-automation` | Exact-target builds, artifacts, provenance, and authorized publication workflow. |
| `v5.2.0` | `feature/upgrade-recovery-policy` | Repeatable install, rollback, upgrade, downgrade, and physical recovery. |
| `v5.3.0` | `feature/platform-security` | Review approved secure boot, flash encryption, and certificate storage. |
| `v5.4.0` | `feature/platform-acceptance` | Target release, security, resource, and recovery acceptance. |

## Expanded functionality — `v6.x`

Defer MQTT, Home Assistant, mDNS, password UX, a read-only USER role, and any
UPS controls until their security and hardware-safety foundations are reviewed.
Configuration backup/restore precedes mDNS/Home Assistant and excludes Wi-Fi
credentials and administrator secrets by default.

| Release | Prospective branch | Scope |
| --- | --- | --- |
| `v6.0.0` | `feature/config-backup-restore` | Reviewed secret-excluding configuration backup/restore. |
| `v6.1.0` | `feature/password-ux` | Improve ADMIN password UX without weakening validation. |
| `v6.2.0` | `feature/mqtt` | Lock and implement bounded broker/security/topic contract. |
| `v6.3.0` | `feature/mdns-discovery` | Reviewed LAN discovery without broader management exposure. |
| `v6.4.0` | `feature/home-assistant-integration` | Read-only Home Assistant discovery and entity validation. |
| `v6.5.0` | `feature/user-role` | Read-only USER role with explicit authorization and recovery. |
| `v6.6.0` | `feature/ups-control-safety` | Lock UPS-control hazard, authorization, and recovery model. |
| `v6.7.0` | `feature/ups-controls` | Implement only approved controls with model-specific validation. |
| `v6.8.0` | `feature/expanded-functionality-acceptance` | Combined security, integration, hardware-safety, recovery, and client acceptance. |

## Guardrails

- Keep the inherited NUT daemon/driver architecture and read-only UPS access.
- Preserve LAN-only HTTPS `443`, read-only NUT `3493`, refused `8080`, and
  ADMIN/CSRF/bearer-token boundaries.
- Use ESP-IDF v6.0.2 on the ESP32-S3 target and validate each slice in
  proportion to its risk.
- Do not flash, OTA-install, reset, push, merge, tag, or release without
  explicit authority.
