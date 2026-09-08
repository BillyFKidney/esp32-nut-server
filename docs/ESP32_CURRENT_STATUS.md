# ESP32-NUT current development status

Read this after [AGENTS.md](../AGENTS.md), confirm it with live Git, and then
load only the document required for the active task. This is the single active
handoff for every agent, including context-limited agents. Do not preload the
archive, source tree, or project chat. Completed release evidence is in
[archive/](archive/README.md), not this startup handoff.

## Snapshot

| Field | Current fact |
| --- | --- |
| Active branch | `feature/optimization` from `main` commit `517980b78`; v2.8.0 is a pushed-candidate workflow until visual browser acceptance closes. |
| Published release | [`v2.7.10`](https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.7.10) is published from tag `6c901bb94`, with `nut-esp32s3-v2.7.10.bin` and `nut-esp32s3-v2.7.10.bin.sha256`. The firmware SHA-256 is `4181ced51f61e8b135f721c94904c9fd06d88affefe2b2cbf87dd00de6e6f0d9`; release evidence is in [archive/v2.7.10/evidence.md](archive/v2.7.10/evidence.md). |
| Active maintenance record | The v2.7.11 browser-update investigation is closed as a remote NGINX configuration incident, not an ESP32-NUT defect. No v2.7.11 firmware will be released. |
| v2.8.0 candidate | The self-contained ADMIN UI now has a persistent appliance header, single-row responsive navigation, compact auto-refresh control, browser-native battery/load meters, full-width hardware diagnostics, no Dashboard logs, pretty Device Status JSON, and a lazy Logs page backed by the unchanged ADMIN 24-entry route with Copy and Download. The redundant certificate/LAN-only and ADMIN-session notices are removed. All pages declare a device-served favicon backed by the canonical tracked NUT logo. Every v1 route/payload and the six-entry `/api/v1/status` log window remain unchanged. The flash-resident 48,089-byte page is streamed through a 768-byte stack buffer instead of allocating a 49,152-byte whole-page heap buffer. |
| v2.8.0 validation | ESP-IDF v6.0.2 reconfigure/build passed; the 1,362,432-byte dirty candidate has SHA-256 `323dc2516d21cc5a39b9cf6af8cab3864151acbd0b08c16ece12ed072f80ee03` and 59% app-slot headroom. It is installed on the authorized `3Dprinter` Agent Tests unit in `app1`; it reports `v2.8.0-evidence-dirty`, update `installed`, HTTPS `443`, NUT `3493`, refused `8080`, and a complete 57-variable NUT poll with `ups.status OL`. Live ADMIN validation passed the removed-notice and favicon checks plus page, status/log, unauthenticated, and invalid-CSRF checks; the live favicon exactly matches the canonical tracked PNG. Project Maintainer Chrome screenshots visually cover desktop, iPhone 16 Pro Max portrait/landscape, iPad Pro, and navigation through all panels on the preceding pushed candidate. Post-refinement browser console/screenshot capture and explicit clipboard/download behavior remain not tested because the browser-control surface was unavailable. Garage did not answer the refreshed post-blink probe and was not modified. |
| v2.7.11 finding | Diagnostic candidate `7a1239084` was built as `v2.7.10-3-g7a1239084` and OTA-installed on the authorized `3Dprinter` unit. It ran from `app1`, reported update state `installed`, and recovered to a full NUT poll with `health: ok` and UPS `OL`. Its clearer browser error exposed `HTTP 413`; the supplied NGINX site configuration confirms that no `client_max_body_size` is set for either ESP32 management site, so NGINX rejected the image before contacting the ESP32. The diagnostic firmware change is not retained for merge. |
| v2.7.10 implementation | Full 24-entry volatile log snapshot route, bounded JSON chunking, click-only Copy Logs/Copy JSON UI, presentation-only `CPS` label mapping, Device Status settings placement, and a macOS build-enforced embedded-JavaScript syntax validator are implemented. The ADMIN page now has a bounded 49,152-byte allocation, and the validator rejects a generated page that does not fit it. |
| v2.7.10 validation | The tagged `v2.7.10` source clean-built with its rendered-page validator and 60% app-partition headroom. The checksum-verified versioned artifact OTA-installed successfully and now reports `v2.7.10`; its post-reboot full NUT poll is `OL`, HTTPS `443` and NUT `3493` respond, and `8080` remains refused. Full API, authorization, Chrome, iPhone Safari, copy, Wi-Fi scan, session-expiry, and responsive-layout evidence is recorded in [archive/v2.7.10/evidence.md](archive/v2.7.10/evidence.md). The clipboard-denial fallback remains implemented but unforced. |
| Target | YD-ESP32-23 / ESP32-S3-WROOM-1-N16R8, ESP-IDF v6.0.2, `esp32s3` |
| Required boundaries | LAN-only HTTPS `443`; read-only NUT `3493`; retired `8080` refused; ADMIN/CSRF and bearer-scope rules preserved |
| Management architecture | `management.c` is the root-policy, HTTPS-lifecycle, and factory-reset orchestration boundary; focused modules own the remaining management concerns |
| Long-term test recovery (August 28) | The previously reported LAN address timed out during recovery preflight. `/dev/cu.usbmodem1101` was present and unowned. The official `v2.7.9` app asset was downloaded, SHA-256-verified against its release sidecar (`c884fff728e143534b1a19b2c91ff9e24a668a6a62a47a8940b34c934457057f`), flashed at `0x10000`, and read-back verified successfully. The Device Operator subsequently reported restored Wi-Fi and retained ADMIN credentials; browser and physical acceptance were not independently repeated in this session. |

## Current objective

Next exact action: in Chrome, reload the updated `3Dprinter` candidate and
confirm the two notices are absent, the NUT favicon appears without a console
404, and Logs Copy/Download plus clipboard fallback behave correctly. If those
checks pass, build/tag/publish the clean `v2.8.0` release and install that exact
artifact; Garage remains untouched until it is independently reachable and
healthy after the September 2 power blink.

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
