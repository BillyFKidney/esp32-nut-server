# ESP32-NUT current development status

Read this after [AGENTS.md](../../AGENTS.md), confirm it with live Git, and then
load only the document required for the active task. This is the single active
handoff for every agent, including context-limited agents. Do not preload the
archive, source tree, or project chat. Completed release evidence is in
[archive/](archive/README.md), not this startup handoff.

## Snapshot

| Field | Current fact |
| --- | --- |
| Active branch | `review/inherited-compatibility-fresh-scan` contains the completed, unversioned `strerror()` portability contract, forced-fallback fixture, and macOS/Ubuntu CI matrix. Both CI jobs passed, and 3Dprinter reports v2.8.17 with healthy normal-libc/NUT service; no release candidate is assigned. The exact next action is to decide whether this coverage justifies a separately scoped structural proposal. |
| v2.8.17 release | This maintenance release aligns the standard-library include group in private time storage, making future persistence maintenance easier to audit without changing NVS schema, timezone behavior, or runtime behavior. The exact tagged ESP-IDF v6.0.2 build is 1,359,301 bytes with SHA-256 `460dac2f6f3f1af5804de79ee458319057954f25ae6fa7205507309785cc07da` and 59% app-slot headroom. It OTA-installed on 3Dprinter; diagnostics report `v2.8.17`, normal Wi-Fi, update `installed`, and full NUT acceptance reports UPS `OL`. [The GitHub release is published](https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.8.17); evidence is in [archive/v2.8.17/evidence.md](../archive/v2.8.17/evidence.md). |
| v2.8.16 release | This maintenance release names the diagnostic seconds-to-microseconds conversion, reducing the chance of future time-unit mistakes without changing disconnect duration bounds, RAM-only state, diagnostic routes, or payloads. Its exact-tag publication artifact is recorded in [archive/v2.8.16/evidence.md](../archive/v2.8.16/evidence.md). |
| v2.8.15 release | This maintenance release gives active and pending Wi-Fi credentials one private NVS helper, preventing the two storage flows from drifting during future maintenance without changing saved keys, validation, provisioning, or recovery behavior. The exact tagged ESP-IDF v6.0.2 build is 1,359,301 bytes with SHA-256 `6e18aeb593bb71dd472a5828e2e52a452a5f6e416928b3164d120c02722758da` and 59% app-slot headroom. It OTA-installed on 3Dprinter and recovered normally on its unchanged `ClubHouse_IoT` configuration; diagnostics report `v2.8.15`, `installed`, healthy NUT and UPS `OL`, with HTTPS `443`/NUT `3493` open and `8080` refused. Evidence is in [archive/v2.8.15/evidence.md](../archive/v2.8.15/evidence.md). |
| v2.8.14 release | This maintenance release prevents a delayed captive-DNS worker from racing teardown of its semaphore and context, making setup-network shutdown more reliable without changing DNS answers, portal routes, or normal station behavior. The exact tagged ESP-IDF v6.0.2 build is 1,359,345 bytes with SHA-256 `c29012b2af9490a06bb283739006f110c846aa8940323ae42cfd03c149038fcb` and 59% app-slot headroom. It OTA-installed on the authorized 3Dprinter device, reports firmware `v2.8.14`, update `installed`, and healthy NUT data with UPS `OL`; HTTPS `443` and read-only NUT `3493` respond while `8080` remains refused. Live setup-AP acceptance confirmed captive DNS, root/status/network routes, unknown-route redirect, credential submission, and recovery to `ClubHouse_IoT`. Evidence is in [archive/v2.8.14/evidence.md](../archive/v2.8.14/evidence.md). |
| v2.8.13 release | This maintenance release isolates private fallback captive-portal lifecycle work from Wi-Fi orchestration, making future portal maintenance safer and easier to review while preserving AP/DNS startup, provisioning validation, recovery, and all unrelated station behavior. The exact tagged build is 1,359,392 bytes with SHA-256 `8501786485bd5331fd86fd359f8bb3987575ac4d841b85731bbbc71cfeabe96c`; evidence is in [archive/v2.8.13/evidence.md](../archive/v2.8.13/evidence.md). |
| Published release | [`v2.8.12`](https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.8.12) is published from the exact tagged build with `nut-esp32s3-v2.8.12.bin` and its SHA-256 sidecar. Firmware SHA-256 is `c25965c3da80d6fbb08be467167e0808482b2a39b08b95453bf433830e0f386b`; release evidence is in [archive/v2.8.12/evidence.md](../archive/v2.8.12/evidence.md). |
| Active maintenance record | The v2.7.11 browser-update investigation is closed as a remote NGINX configuration incident, not an ESP32-NUT defect. No v2.7.11 firmware will be released. |
| v2.8.0 candidate | The self-contained ADMIN UI now has a persistent appliance header, single-row responsive navigation, compact auto-refresh control, browser-native battery/load meters, full-width hardware diagnostics, no Dashboard logs, pretty Device Status JSON, and a lazy Logs page backed by the unchanged ADMIN 24-entry route with Copy and Download. The redundant certificate/LAN-only and ADMIN-session notices are removed. All pages declare a device-served favicon backed by the canonical tracked NUT logo. Every v1 route/payload and the six-entry `/api/v1/status` log window remain unchanged. The flash-resident 48,089-byte page is streamed through a 768-byte stack buffer instead of allocating a 49,152-byte whole-page heap buffer. |
| v2.8.0 validation | The exact annotated-tag build from `v2.8.0` passed ESP-IDF v6.0.2 reconfigure/build and `idf.py size`; the 1,362,432-byte artifact has SHA-256 `9bc140383d93d140c46da319b95d58db15968d3a91ad167ef90a797501c781f8` and 59% app-slot headroom. It is OTA-installed on the authorized `3Dprinter` Agent Tests unit in `app0`; diagnostics report firmware `v2.8.0`, update `installed`, HTTPS `443`, NUT `3493`, refused `8080`, and a post-reboot complete 57-variable NUT poll with `ups.status OL`. Live ADMIN validation passed the removed-notice and favicon checks plus page, status/log, unauthenticated, and invalid-CSRF checks; the live favicon exactly matches the canonical tracked PNG. The Project Maintainer's final Chrome check accepted the UI as great and the remaining icon issue as acceptable. The screenshot's two console 404s were requests to external `c.1password.com/richicons/...` images for 1Password entries, not the ESP32 `/favicon.ico`. Clipboard/download behavior remains not tested. Garage did not answer the refreshed post-blink probe and was not modified. |
| v2.8.1 release | This maintenance release adds no product features and preserves the released management, authorization, NUT, UPS, and service contracts. The exact tagged ESP-IDF v6.0.2 build is 1,362,080 bytes with SHA-256 `ebb20c6a047c42498e2d4d382b44bc91e2a8e3d4413d44999eab8f6fc0fc11f3` and 59% app-slot headroom. It is OTA-installed on 3Dprinter at `192.168.40.88` in `app0`, reports firmware `v2.8.1` and update `installed`, has a healthy 57-variable NUT poll with UPS `OL`, retains HTTPS `443`/NUT `3493`, and refuses `8080`. The repaired FQDN passes trusted HTTPS root, unauthenticated 401, and scoped diagnostic-token status acceptance. [The GitHub release is published](https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.8.1); evidence is in [archive/v2.8.1/evidence.md](../archive/v2.8.1/evidence.md). |
| v2.8.3 release | This maintenance release unifies duplicated OTA and diagnostic token-store lifecycle behind a private store while preserving token NVS layout, verifier-only storage, token scopes, ADMIN/CSRF routes, and external payloads. The exact tagged ESP-IDF v6.0.2 build is 1,359,808 bytes with SHA-256 `3ba9ba14e38af156c29566cf635e4c0087a2699dbbf1b861b140bb1daeb186ac` and 59% app-slot headroom. It is OTA-installed on the authorized Agent Tests unit, reports firmware `v2.8.3` and update `installed`, has a healthy 57-variable NUT poll with UPS `OL`, retains HTTPS `443`/NUT `3493`, and refuses `8080`. [The GitHub release is published](https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.8.3); evidence is in [archive/v2.8.3/evidence.md](../archive/v2.8.3/evidence.md). |
| v2.8.4 release | This maintenance release extracts OTA receive/write/verify/abort work into a private helper and names the existing reboot task configuration, without changing authorization, responses, NVS result transitions, partition selection, or restart sequencing. The exact tagged ESP-IDF v6.0.2 build is 1,359,824 bytes with SHA-256 `3abea36c7ad62a97033edb7277c913da2f83eccc4847f84f41bd84c748ce5987` and 59% app-slot headroom. It is OTA-installed on the authorized Agent Tests unit, reports firmware `v2.8.4` and update `installed`, has a healthy 57-variable NUT poll with UPS `OL`, retains HTTPS `443`/NUT `3493`, and refuses `8080`. [The GitHub release is published](https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.8.4); evidence is in [archive/v2.8.4/evidence.md](../archive/v2.8.4/evidence.md). |
| v2.8.5 release | This maintenance release isolates Wi-Fi scan record collection/deduplication, restores AP-list cleanup after record-retrieval failure, and removes an unreachable duplicate physical button-release loop without changing provisioning, credentials, synchronization, or routes. The exact tagged ESP-IDF v6.0.2 build is 1,359,760 bytes with SHA-256 `809ea0a7ac2570cf902ae42bafcca9449fd61a3e7286983150d4fb5e88a4deb5` and 59% app-slot headroom. Authenticated live Wi-Fi scanning returned nine networks. It is OTA-installed on the authorized Agent Tests unit, reports firmware `v2.8.5` and update `installed`, has a healthy 57-variable NUT poll with UPS `OL`, retains HTTPS `443`/NUT `3493`, and refuses `8080`. [The GitHub release is published](https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.8.5); evidence is in [archive/v2.8.5/evidence.md](../archive/v2.8.5/evidence.md). |
| v2.8.6 release | This maintenance release isolates private time-configuration persistence and timezone policy from SNTP lifecycle operations without changing the NVS schema, defaults, synchronization, routes, or public status behavior. The exact tagged ESP-IDF v6.0.2 build is 1,359,776 bytes with SHA-256 `241f8aa1bf4e8a59e8723f896932224feedb6bc6ff0e8bc62c451f2fd4a46b7c` and 59% app-slot headroom. It is OTA-installed on the authorized Agent Tests unit, reports firmware `v2.8.6`, update `installed`, and populated time configuration; HTTPS `443` and read-only NUT `3493` respond, `8080` is refused, and the post-reboot NUT poll returns 57 variables with UPS `OL`. [The GitHub release is published](https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.8.6); evidence is in [archive/v2.8.6/evidence.md](../archive/v2.8.6/evidence.md). |
| v2.7.11 finding | Diagnostic candidate `7a1239084` was built as `v2.7.10-3-g7a1239084` and OTA-installed on the authorized `3Dprinter` unit. It ran from `app1`, reported update state `installed`, and recovered to a full NUT poll with `health: ok` and UPS `OL`. Its clearer browser error exposed `HTTP 413`; the supplied NGINX site configuration confirms that no `client_max_body_size` is set for either ESP32 management site, so NGINX rejected the image before contacting the ESP32. The diagnostic firmware change is not retained for merge. |
| v2.7.10 implementation | Full 24-entry volatile log snapshot route, bounded JSON chunking, click-only Copy Logs/Copy JSON UI, presentation-only `CPS` label mapping, Device Status settings placement, and a macOS build-enforced embedded-JavaScript syntax validator are implemented. The ADMIN page now has a bounded 49,152-byte allocation, and the validator rejects a generated page that does not fit it. |
| v2.7.10 validation | The tagged `v2.7.10` source clean-built with its rendered-page validator and 60% app-partition headroom. The checksum-verified versioned artifact OTA-installed successfully and now reports `v2.7.10`; its post-reboot full NUT poll is `OL`, HTTPS `443` and NUT `3493` respond, and `8080` remains refused. Full API, authorization, Chrome, iPhone Safari, copy, Wi-Fi scan, session-expiry, and responsive-layout evidence is recorded in [archive/v2.7.10/evidence.md](archive/v2.7.10/evidence.md). The clipboard-denial fallback remains implemented but unforced. |
| Target | YD-ESP32-23 / ESP32-S3-WROOM-1-N16R8, ESP-IDF v6.0.2, `esp32s3` |
| Required boundaries | LAN-only HTTPS `443`; read-only NUT `3493`; retired `8080` refused; ADMIN/CSRF and bearer-scope rules preserved |
| Management architecture | `management.c` is the root-policy, HTTPS-lifecycle, and factory-reset orchestration boundary; focused modules own the remaining management concerns |
| Long-term test recovery (August 28) | The previously reported LAN address timed out during recovery preflight. `/dev/cu.usbmodem1101` was present and unowned. The official `v2.7.9` app asset was downloaded, SHA-256-verified against its release sidecar (`c884fff728e143534b1a19b2c91ff9e24a668a6a62a47a8940b34c934457057f`), flashed at `0x10000`, and read-back verified successfully. The Device Operator subsequently reported restored Wi-Fi and retained ADMIN credentials; browser and physical acceptance were not independently repeated in this session. |

## Current objective

The v2.8.6 time-configuration review is released from `main`
`22e222bedebc1347ccfefe8849130479b52bc19a` as
[v2.8.6](https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.8.6).
Its one-item tracker is complete: private persistence and timezone policy now
live in `time-config-storage.c`, while `time_config.c` retains SNTP lifecycle
and public operations. Independent review preserved the NVS schema, defaults,
timezone mapping, synchronization sequencing, and route behavior. Both
1Password environments passed redacted presence and format checks; Garage was
not modified. The tagged artifact passed exact ESP-IDF v6.0.2 build/size,
scoped OTA, post-reboot version and update-state checks, populated time status,
HTTPS `443`, read-only NUT `3493`, refused `8080`, and a healthy 57-variable
`OL` NUT poll. The exact next action is to select the next dedicated review
from the explicit v2.8.2 `SKIP` debt; it remains `v2.8.Z` until that review
defines its concrete release boundary.

The v2.8.7 preparation scanner found `src/management-session.c` clean across
all 68 checks. It then found one moderate, bounded duplication in the runtime
log path: `management-log.c` and `management-log-routes.c` independently map
the same `E`/`W`/`D`/`V` levels to payload names. The active one-item tracker
proposes exposing that existing mapping through the management-log module and
removing the route-local copy, with no payload or authorization changes. The
fixer and independent reviewer completed the one-item extraction: the
management-log module now owns the mapping for both serializers, preserving all
payload values. The tracker is complete. The exact-tag artifact passed clean
ESP-IDF v6.0.2 build/size validation, scoped OTA, post-reboot pinned
diagnostics, authenticated six-entry status and 24-entry full-log contracts,
HTTPS `443`, read-only NUT `3493`, refused `8080`, and a healthy 57-variable
`OL` NUT poll. [v2.8.7 is published](https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.8.7).
The exact next action is to select the next focused review from the v2.8.2
`SKIP` debt as `v2.8.Z`.

The v2.8.8 preparation scan found `src/management-device-config.c` clean across
all 68 checks. It then found one moderate, bounded routine-size issue in
`management_extract_form_value()` in `management-http.c`: its name and value
paths independently decode form components. The active one-item tracker
authorized extracting that private decoder while preserving every current
malformed-input, percent-decoding, plus-to-space, truncation, route, and
security behavior. Independent review confirmed the equivalence, including
incomplete escapes and embedded NUL behavior; the tracker is complete. The
exact-tag artifact passed clean ESP-IDF v6.0.2 build/size validation, scoped OTA,
post-reboot pinned diagnostics, authenticated ADMIN form/log contracts, HTTPS
`443`, read-only NUT `3493`, refused `8080`, and a healthy 57-variable `OL`
NUT poll. [v2.8.8 is published](https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.8.8).
The exact next action is the next `v2.8.Z` review from the v2.8.2 `SKIP` debt.

The released `v2.8.10` review fixed one moderate boundary-output issue in
`management-device-routes.c`: accepted device names containing quotes or
backslashes now serialize through the existing bounded JSON helpers. This
keeps valid administrator-selected display names consumable by the management
UI and API clients without changing names, persistence, response fields, or
ADMIN/CSRF behavior. Independent review, clean exact-tag build with embedded
version inspection, scoped OTA, authenticated ADMIN acceptance, service-boundary
checks, and a 57-variable `OL` NUT poll passed. The exact next action is the
next `v2.8.Z` review from the v2.8.2 `SKIP` debt.

The released `v2.8.11` review fixed one moderate browser-OTA boundary issue in
`management-ota-routes.c`: after valid ADMIN/CSRF verification, the browser
install route accepts missing or alternate content types and can reach OTA work
before image validation rejects the request. The active tracker limits the
fix to the existing exact content-type rejection helper before OTA state,
partition writes, and reboot scheduling. Independent review, clean exact-tag
build with embedded-version inspection, scoped OTA, live browser rejection
checks, authenticated ADMIN acceptance, service-boundary checks, and a
57-variable `OL` NUT poll passed. The exact next action is the next `v2.8.Z`
review from the v2.8.2 `SKIP` debt.

The next scanner found one moderate duplicate creation workflow in
`management-token-routes.c`. The active tracker limits the fix to a private,
explicitly configured helper that preserves separate token stores, scopes,
capacities, errors, ADMIN/CSRF, time gating, one-time plaintext responses, and
zeroization. The exact next action is the standing-approved fixer followed by
both token-family lifecycle and scope-isolation acceptance.

Read-only follow-up scans found `management-wifi-routes.c` and
`management-certificates.c` clean across all 68 checks. They found one
moderate duplicated predicate in `management-credentials.c`: verification
repeats the existing private current-format checks for record size, version,
and iteration bounds. The active tracker authorizes replacing that duplicate
only inside its existing `credential_result == ESP_OK` guard. The
standing-approved fixer reused the existing private predicate only inside that
guard. Independent review confirmed that schema, migration separation,
iteration bounds, PBKDF2, constant-time comparison, handle closure, and all
zeroization paths remain unchanged; the tracker is complete. An initial
incremental artifact retained v2.8.8 metadata and was never published. A clean
reconfigure produced an image inspected to embed v2.8.9 before successful OTA.
The corrected artifact passed pinned diagnostics, ADMIN/CSRF contracts, HTTPS
`443`, read-only NUT `3493`, refused `8080`, and a healthy 57-variable `OL`
NUT poll. [v2.8.9 is published](https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.8.9).
The exact next action is the next `v2.8.Z` review from the v2.8.2 `SKIP` debt.

## Read only when needed

| Need | Document |
| --- | --- |
| Active releases and branch scope | [ESP32_DEVELOPMENT_PLAN.md](ESP32_DEVELOPMENT_PLAN.md) |
| v2.8.0 implementation and acceptance | [ESP32_V2_8_0_IMPLEMENTATION_PLAN.md](ESP32_V2_8_0_IMPLEMENTATION_PLAN.md) |
| v2.7.10 status UI implementation and acceptance | [ESP32_V2_7_10_STATUS_UI_POLISH_SPEC.md](ESP32_V2_7_10_STATUS_UI_POLISH_SPEC.md) |
| v2.7.11 browser-local OTA fix | [ESP32_V2_7_11_BROWSER_FIRMWARE_UPDATES.md](ESP32_V2_7_11_BROWSER_FIRMWARE_UPDATES.md) |
| Hardware, LAN, COM, build, flash, or OTA | [ESP32_PREFLIGHT.md](ESP32_PREFLIGHT.md) |
| Authority for physical, destructive, or external actions | [ESP32_DEVELOPMENT_ROLES.md](ESP32_DEVELOPMENT_ROLES.md) |
| Security and authorization boundaries | [ESP32_SECURITY.md](ESP32_SECURITY.md) |
| Completed management/Wi-Fi refactoring architecture | [ESP32_REFACTORING_PLAN.md](ESP32_REFACTORING_PLAN.md) |
| Released v2.7.8 status UI evidence | [archive/v2.7.8/evidence.md](archive/v2.7.8/evidence.md) |
| Released v2.7.7 factory-reset evidence | [archive/v2.7.7/evidence.md](archive/v2.7.7/evidence.md) |
| Released v2.7.6 replacement-UPS reprobe | [archive/v2.7.6/ESP32_V2_7_6_UPS_REPLACEMENT_SPEC.md](archive/v2.7.6/ESP32_V2_7_6_UPS_REPLACEMENT_SPEC.md) |
| Released v2.7.5 compatibility hardening | [archive/v2.7.5/evidence.md](archive/v2.7.5/evidence.md) |
| Released v2.7.4 APC compatibility evidence | [archive/v2.7.4/evidence.md](archive/v2.7.4/evidence.md) |
| Released v2.7.3 implementation and acceptance contract | [archive/v2.7.3/ESP32_V2_7_3_STALE_TIMEOUT_SPEC.md](archive/v2.7.3/ESP32_V2_7_3_STALE_TIMEOUT_SPEC.md) |
| Released v2.7.2 acceptance evidence | [archive/v2.7.2/evidence.md](archive/v2.7.2/evidence.md) |
| Released v2.7.1 refactoring and release evidence | [archive/README.md](archive/README.md) |

Never record credentials, cookies, API tokens, private keys, or Authorization
headers here.
