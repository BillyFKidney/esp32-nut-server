# ESP32-NUT current development status

Read this after [AGENTS.md](../AGENTS.md), confirm it with live Git, and then
load only the document required for the active task. This is the single active
handoff for every agent, including context-limited agents. Do not preload the
archive, source tree, or project chat. Completed release evidence is in
[archive/](archive/README.md), not this startup handoff.

## Snapshot

| Field | Current fact |
| --- | --- |
| Canonical branch | `main`; active maintenance work is `fix/browser-firmware-updates` from `f258b7b6a` for v2.7.11. |
| Published release | [`v2.7.10`](https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.7.10) is published from tag `6c901bb94`, with `nut-esp32s3-v2.7.10.bin` and `nut-esp32s3-v2.7.10.bin.sha256`. The firmware SHA-256 is `4181ced51f61e8b135f721c94904c9fd06d88affefe2b2cbf87dd00de6e6f0d9`; release evidence is in [archive/v2.7.10/evidence.md](archive/v2.7.10/evidence.md). |
| Active implementation | v2.7.11 fixes browser-local firmware-check re-entry/error reporting and bounds the post-install reconnect loop before v2.8.0 optimization begins. |
| v2.7.11 target state | Clean candidate `7a1239084` was built as `v2.7.10-3-g7a1239084` and OTA-installed on the authorized `3Dprinter` unit. It is running from `app1`, reports update state `installed`, and recovered to a full NUT poll with `health: ok` and UPS `OL`. The authenticated browser check/install acceptance remains pending. |
| v2.7.10 implementation | Full 24-entry volatile log snapshot route, bounded JSON chunking, click-only Copy Logs/Copy JSON UI, presentation-only `CPS` label mapping, Device Status settings placement, and a macOS build-enforced embedded-JavaScript syntax validator are implemented. The ADMIN page now has a bounded 49,152-byte allocation, and the validator rejects a generated page that does not fit it. |
| v2.7.10 validation | The tagged `v2.7.10` source clean-built with its rendered-page validator and 60% app-partition headroom. The checksum-verified versioned artifact OTA-installed successfully and now reports `v2.7.10`; its post-reboot full NUT poll is `OL`, HTTPS `443` and NUT `3493` respond, and `8080` remains refused. Full API, authorization, Chrome, iPhone Safari, copy, Wi-Fi scan, session-expiry, and responsive-layout evidence is recorded in [archive/v2.7.10/evidence.md](archive/v2.7.10/evidence.md). The clipboard-denial fallback remains implemented but unforced. |
| Target | YD-ESP32-23 / ESP32-S3-WROOM-1-N16R8, ESP-IDF v6.0.2, `esp32s3` |
| Required boundaries | LAN-only HTTPS `443`; read-only NUT `3493`; retired `8080` refused; ADMIN/CSRF and bearer-scope rules preserved |
| Management architecture | `management.c` is the root-policy, HTTPS-lifecycle, and factory-reset orchestration boundary; focused modules own the remaining management concerns |
| Long-term test recovery (August 28) | The previously reported LAN address timed out during recovery preflight. `/dev/cu.usbmodem1101` was present and unowned. The official `v2.7.9` app asset was downloaded, SHA-256-verified against its release sidecar (`c884fff728e143534b1a19b2c91ff9e24a668a6a62a47a8940b34c934457057f`), flashed at `0x10000`, and read-back verified successfully. The Device Operator subsequently reported restored Wi-Fi and retained ADMIN credentials; browser and physical acceptance were not independently repeated in this session. |

## Current objective

Next exact action: perform the authenticated ADMIN-browser firmware check on
the authorized `3Dprinter` candidate, then validate the bounded browser install
reconnect flow before handing browser acceptance to the Device Operator.

## Read only when needed

| Need | Document |
| --- | --- |
| Active releases and branch scope | [ESP32_DEVELOPMENT_PLAN.md](ESP32_DEVELOPMENT_PLAN.md) |
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
