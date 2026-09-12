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
4. Clean-reconfigure and build the firmware from that tagged commit for the
   intended target. Inspect the built image with `esptool image_info` and
   confirm its embedded application version matches the release tag.
5. Generate and verify the SHA-256 checksum against that version-verified
   `build/nut-esp32s3.bin` output.
6. Publish the versioned asset `nut-esp32s3-vX.Y.Z.bin` and its matching
   `nut-esp32s3-vX.Y.Z.bin.sha256` sidecar. Record both the source build path
   and published asset path in release evidence, and link that evidence.
7. Update [ESP32_CURRENT_STATUS.md](ESP32_CURRENT_STATUS.md) with the
   publication, validation state, and next action.

Every GitHub release body must use this exact Markdown structure:

```markdown
## Benefit

## What changed

## Verification

## Artifact
```

Lead with the practical upgrade benefit, then state the implementation change
and preserved contracts. List only checks actually performed, and record the
versioned asset and SHA-256. Do not present internal refactoring alone as the
reason to upgrade or replace these headings with unstructured prose.

## Published baseline

`v2.8.0` through `v2.8.17` are published and target-tested. The
fresh bounded `src/common/strerror.c` review confirms inherited portability
debt still requires a cross-platform errno/diagnostic compatibility contract
and fixtures before it receives a release version.

## Operational Management completion — `v2.x`

The locked requirements remain in
[ESP32_DEVELOPMENT_MILESTONE_QA_OPERATIONAL_MANAGEMENT.md](ESP32_DEVELOPMENT_MILESTONE_QA_OPERATIONAL_MANAGEMENT.md).
The ADMIN console, LAN-only HTTPS, read-only NUT access, and physical-recovery
boundaries remain in force. The remaining umbrella-milestone slices are:

| Release | Prospective branch | Required outcome |
| --- | --- | --- |
| `v2.8.0` | Released | [Appliance UI and bounded-resource optimization](archive/v2.8.0/evidence.md): retained every v1 API contract, added the responsive appliance UI and lazy 24-entry Logs page, streamed the ADMIN page without a whole-page heap allocation, and passed clean tagged build, OTA, browser, service, and full-NUT validation. |
| `v2.8.1` | `feature/v2.8.1-code-quality` | [Code-quality maintenance](archive/v2.8.1/evidence.md): preserve all released behavior while applying narrowly reviewed code-quality improvements. Require a clean exact-tag build, certificate-pinned scoped OTA, repaired-proxy browser and ADMIN/CSRF acceptance, HTTPS `443`, read-only NUT `3493`, refused `8080`, and a post-reboot full successful NUT poll before publication. |
| `v2.8.2` | Released | Fresh 68-check review of all 108 tracked `src/` files is complete: seven narrowly behavior-preserving hygiene/`mountFS` fixes passed independent review; 62 security-, compatibility-, lifecycle-, or architecture-sensitive findings remain explicit `SKIP` debt for dedicated slices. The exact-tag build, scoped OTA, service-boundary checks, and healthy full-NUT poll passed before [publication](https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.8.2). |
| `v2.8.3` | Released | [Token-store code-review maintenance](ESP32_API_TOKEN_STORAGE_CONTRACT.md): duplicated OTA/diagnostic token lifecycle now shares a private store while preserving NVS layout and authorization isolation. Exact-tag ESP-IDF v6.0.2 build, live create/list/delete/revocation checks, scope isolation, scoped OTA, service boundaries, and healthy full-NUT acceptance passed before [publication](https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.8.3); see [release evidence](../archive/v2.8.3/evidence.md). |
| `v2.8.4` | Released | [OTA route code-review maintenance](ESP32_OTA_ROUTE_CONTRACT.md): split the OTA request orchestration without changing authorization, image, persistence, response, partition, or reboot behavior. Independent scan/review, exact-tag ESP-IDF v6.0.2 build, scoped OTA, service-boundary checks, and healthy full-NUT acceptance passed before [publication](https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.8.4); see [release evidence](../archive/v2.8.4/evidence.md). |
| `v2.8.5` | Released | [Wi-Fi scan code-review maintenance](ESP32_WIFI_SCAN_CONTRACT.md): split the scan lifecycle, restore cleanup symmetry, and remove proven unreachable button-release logic without changing synchronization, provisioning, credential, or route behavior. Independent scan/review, exact-tag ESP-IDF v6.0.2 build, authenticated Wi-Fi scan, scoped OTA, service-boundary checks, and healthy full-NUT acceptance passed before [publication](https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.8.5); see [release evidence](../archive/v2.8.5/evidence.md). |
| `v2.8.6` | Released | [Time-configuration code-review maintenance](ESP32_TIME_CONFIG_CONTRACT.md): split private NVS persistence and timezone policy from SNTP lifecycle and public operations without changing time, synchronization, or route contracts. Independent scan/review, exact-tag ESP-IDF v6.0.2 build/size validation, scoped OTA, time-status, service-boundary checks, and a healthy full-NUT acceptance passed before [publication](https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.8.6); see [release evidence](../archive/v2.8.6/evidence.md). |
| `v2.8.7` | Released | [Management-log code-review maintenance](ESP32_CURRENT_STATUS.md): centralized the duplicated private log-level mapping used by runtime-log capture and full-log routes while preserving every response name and payload contract. Independent scan/review, exact-tag ESP-IDF v6.0.2 build/size validation, scoped OTA, authenticated ADMIN log contracts, service-boundary checks, and a healthy full-NUT acceptance passed before [publication](https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.8.7); see [release evidence](../archive/v2.8.7/evidence.md). |
| `v2.8.8` | Released | [Management-HTTP code-review maintenance](ESP32_CURRENT_STATUS.md): extracted duplicate private form-component decode loops while preserving malformed-input, percent-decoding, plus-to-space, truncation, route, and security behavior. Independent scan/review, exact-tag ESP-IDF v6.0.2 build/size validation, scoped OTA, authenticated ADMIN form/log contracts, service-boundary checks, and a healthy full-NUT acceptance passed before [publication](https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.8.8); see [release evidence](../archive/v2.8.8/evidence.md). |
| `v2.8.9` | Released | [Credential-format code-review maintenance](ESP32_CURRENT_STATUS.md): centralized current-format validation so future credential-security maintenance cannot let stored-record and password-verification rules drift. Independent scan/review, clean-reconfigured exact-tag ESP-IDF v6.0.2 build with embedded-version inspection, scoped OTA, authenticated ADMIN contracts, service-boundary checks, and a healthy full-NUT acceptance passed before [publication](https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.8.9); see [release evidence](../archive/v2.8.9/evidence.md). |
| `v2.8.10` | Released | [Device-response JSON maintenance](ESP32_CURRENT_STATUS.md): valid display names with quotes or backslashes now return reliable JSON to management UI and API clients. The bounded serializer preserves accepted names, persistence, response fields, hostname behavior, and ADMIN/CSRF protections. Independent scan/review, clean-reconfigured exact-tag ESP-IDF v6.0.2 build with embedded-version inspection, scoped OTA, authenticated ADMIN acceptance, service-boundary checks, and a healthy full-NUT acceptance passed before [publication](https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.8.10); see [release evidence](../archive/v2.8.10/evidence.md). |
| `v2.8.11` | Released | [Browser OTA boundary maintenance](ESP32_CURRENT_STATUS.md): malformed browser uploads now fail predictably before any inactive-partition work, protecting the update path while preserving ADMIN/CSRF precedence, bearer-agent OTA, and established error contracts. Independent scan/review, clean-reconfigured exact-tag ESP-IDF v6.0.2 build with embedded-version inspection, scoped OTA, live browser rejection checks, authenticated ADMIN acceptance, service-boundary checks, and a healthy full-NUT acceptance passed before [publication](https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.8.11); see [release evidence](../archive/v2.8.11/evidence.md). |
| `v2.8.12` | Released | [Token-workflow maintenance](ESP32_CURRENT_STATUS.md): API and diagnostic issuance now share one fixed internal workflow, reducing future drift between security-sensitive paths without broadening either credential's authority. Independent scan/review, clean-reconfigured exact-tag ESP-IDF v6.0.2 build with embedded-version inspection, scoped OTA, full temporary token lifecycle/scope-isolation checks with revocation, authenticated ADMIN acceptance, service-boundary checks, and a healthy full-NUT acceptance passed before [publication](https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.8.12); see [release evidence](../archive/v2.8.12/evidence.md). |
| `v2.8.13` | Released | [Fallback portal lifecycle maintenance](../archive/v2.8.13/evidence.md): the private AP/DNS/portal lifecycle now has one focused owner, reducing regression risk in future Wi-Fi work while preserving AP+STA/open-AP/DHCP-DNS behavior, critical-section/event-bit ordering, failure cleanup, task lifetime, provisioning validation, and station recovery. Independent scan/review, exact-tag ESP-IDF v6.0.2 build, scoped OTA, live captive-portal routes and malformed-input rejection, credential recovery, service-boundary checks, and a healthy full-NUT acceptance passed before publication. |
| `v2.8.14` | Released | [Captive-DNS lifecycle maintenance](../archive/v2.8.14/evidence.md): a worker that outlives the retained one-second graceful-stop window cleans up its own state rather than racing semaphore or context destruction. This makes setup-network shutdown more reliable while preserving DNS answers, portal routes, provisioning, and normal station behavior. Independent scan/review, exact-tag ESP-IDF v6.0.2 build, scoped OTA, live captive-DNS and portal acceptance, credential recovery, service-boundary checks, and healthy NUT acceptance passed before publication. |
| `v2.8.15` | Released | [Wi-Fi credential lifecycle maintenance](../archive/v2.8.15/evidence.md): active and pending records share fixed private NVS helpers, preventing future storage-flow drift while preserving separate keys, schema, validation, errors, zeroization, provisioning, and recovery behavior. Independent scan/review, exact-tag ESP-IDF v6.0.2 build, scoped OTA, unchanged-station recovery, service-boundary checks, and healthy NUT acceptance passed before publication. |
| `v2.8.16` | Released | [Diagnostic simulation maintenance](../archive/v2.8.16/evidence.md): names the duration conversion so future time-unit changes are less error-prone while preserving bounded RAM-only disconnect simulation, diagnostic timing, routes, and payloads. Independent scan/review, exact-tag build, scoped OTA, service-boundary checks, and full-NUT acceptance passed before publication. |
| `v2.8.17` | Released | [Time-storage import-order maintenance](../archive/v2.8.17/evidence.md): aligns the standard-library include group, making the private storage module easier to audit while preserving NVS persistence, timezone behavior, and runtime contracts. Independent scan/review, exact-tag build, scoped OTA, service-boundary checks, and full-NUT acceptance passed before publication. |
| Deferred | `review/inherited-compatibility-fresh-scan` | Fresh bounded review of `src/common/strerror.c` confirms its 526-line conditional errno table is inherited portability behavior. The unversioned [compatibility contract](ESP32_STRERROR_COMPATIBILITY_CONTRACT.md) and forced-fallback host fixture pass; a 3Dprinter normal-libc regression also passed. Any split or modernization still requires a supported target matrix, not an unscoped hygiene release. |

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
| `v2.7.9` | Released | [Device identity and log level](archive/v2.7.9/ESP32_V2_7_9_DEVICE_CONFIGURATION_SPEC.md): configurable `device_name`, safe derived hostname, reboot-persistent log-level dropdown, and the validated status-response stack-pressure repair. |
| `v2.7.10` | Released | [Status UI polish and full-log retrieval evidence](archive/v2.7.10/evidence.md): display-only `CPS` mapping, Device Status settings placement, permanently visible raw JSON, canonical-response `Copy JSON`, click-only `Copy Logs`, bounded ADMIN full-log route, and rendered-page allocation regression repair. The clipboard-denial fallback is implemented but was not directly forced; all primary browser, API, service-boundary, OTA, and full-NUT-poll acceptance passed. |
| `v2.7.11` | Closed — no firmware release | [Browser-based firmware-update incident](ESP32_V2_7_11_BROWSER_FIRMWARE_UPDATES.md): browser uploads failed because the remote NGINX sites retained the stock 1 MiB request-body limit, which rejected the image with `HTTP 413` before it reached the ESP32. The diagnostic candidate was not retained; remediation is the documented host-specific NGINX configuration. |

Factory-reset state clearing remains the final persisted-state-clearing slice;
the following identity and presentation slices do not expand its reset scope.

## API v2, tokens, and Production OTA — `v3.x`

- Begin the major-version family with an explicit API v2 and token-contract
  review. This is the authorized compatibility boundary for simplifying or
  removing v1 payload fields, reducing response/allocation pressure, and
  improving bounded request processing. Inventory every ADMIN, Agent, and
  browser consumer; define token scopes, migration/rollback behavior, and
  measurable resource budgets before implementation.
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
| `v3.0.0` | `feature/api-v2-tokens` | [API v2 and token contracts](ESP32_V3_0_API_V2_TOKENS_PLAN.md): the intentional compatibility boundary for management and Agent APIs. Inventory v1 consumers, define route/payload/token changes, reduce bounded response/allocation cost, and validate processing/resource budgets and rollback. The v3.0 planning agent must read the linked plan; detailed evidence is loaded only when that plan directs it. |
| `v3.1.0` | `feature/local-ca-trust` | Local-CA trust and provisioning model. |
| `v3.2.0` | `feature/signed-update-metadata` | Signed release metadata or supported signed-image strategy. |
| `v3.3.0` | `feature/remote-update-client` | Certificate-validated remote check/download with manual approval. |
| `v3.4.0` | `feature/scheduled-updates` | Opt-in check scheduling; automatic installation remains disabled. |
| `v3.5.0` | `feature/production-ota-acceptance` | Validate authorization, source resistance, rollback, recovery, and definition of done. |

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
