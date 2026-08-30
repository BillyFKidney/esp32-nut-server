# ESP32-NUT current development status

Read this after [AGENTS.md](../AGENTS.md), confirm it with live Git, and then
load only the document required for the active task. This is the single active
handoff for every agent, including context-limited agents. Do not preload the
archive, source tree, or project chat. Completed release evidence is in
[archive/](archive/README.md), not this startup handoff.

## Snapshot

| Field | Current fact |
| --- | --- |
| Canonical branch | `main` at `4dce455fc`; active worktree is `feature/status-ui-polish` from that base, with uncommitted v2.7.10 implementation, planning, and v2.7.9 provenance reconciliation changes |
| Published release | [`v2.7.9`](https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.7.9) tag points to `bb92582b7`; release evidence is in [archive/v2.7.9/evidence.md](archive/v2.7.9/evidence.md) |
| Active implementation | v2.7.9 device identity, retained log level, and status-response stack-pressure repair are published and target-accepted. v2.7.10 adds ADMIN full-log retrieval and Device Status polish only; it is not released. Its repaired dirty candidate is installed through the scoped OTA route on the authorized `3Dprinter` test unit. |
| v2.7.10 implementation | Full 24-entry volatile log snapshot route, bounded JSON chunking, click-only Copy Logs/Copy JSON UI, presentation-only `CPS` label mapping, Device Status settings placement, and a macOS build-enforced embedded-JavaScript syntax validator are implemented. The ADMIN page now has a bounded 49,152-byte allocation, and the validator rejects a generated page that does not fit it. |
| v2.7.10 validation | `git diff --check`; ESP-IDF v6.0.2 `esp32s3` reconfigure/build with validator execution; ADMIN page rendered at 43,137 UTF-8 bytes within its 49,152-byte allocation; 60% app-partition headroom. Certificate-pinned 1Password-managed test credentials verified diagnostic status, OTA installation, post-reboot Wi-Fi/NTP retention, running OTA slot, full NUT poll (`OL`), HTTPS `443`, NUT `3493`, and refused `8080`. ADMIN login and status were `200`; the authenticated ADMIN page was `200` with its full-log control and without the fallback; full-log retrieval was `200`, reported the retained capacity/window correctly, and diagnostic bearer access was `401`; the idle session continued counting down across the read. Manual Chrome and iPhone Safari acceptance passed the `CPS` display mapping with raw JSON unchanged, `Copy JSON`, and `Copy Logs`: the latter returned the full 24-entry ring oldest-to-newest, then advanced as expected when newer entries displaced the oldest. Screenshots confirm Device Status places the copy controls before the settings form and continuously visible raw JSON; Dashboard and Device Status render well at desktop and phone viewports, and Wi-Fi scan/selection still works without saving a network change. After the 15-minute idle timeout, the manual Copy Logs attempt returned to normal sign-in before copying content. The observed NUT client-write resets to another LAN host did not make the target stale: its status still reported NUT `ok` and UPS `OL`. The clipboard-denial fallback is implemented and remains the only unexercised UI contingency. |
| Target | YD-ESP32-23 / ESP32-S3-WROOM-1-N16R8, ESP-IDF v6.0.2, `esp32s3` |
| Required boundaries | LAN-only HTTPS `443`; read-only NUT `3493`; retired `8080` refused; ADMIN/CSRF and bearer-scope rules preserved |
| Management architecture | `management.c` is the root-policy, HTTPS-lifecycle, and factory-reset orchestration boundary; focused modules own the remaining management concerns |
| Long-term test recovery (August 28) | The previously reported LAN address timed out during recovery preflight. `/dev/cu.usbmodem1101` was present and unowned. The official `v2.7.9` app asset was downloaded, SHA-256-verified against its release sidecar (`c884fff728e143534b1a19b2c91ff9e24a668a6a62a47a8940b34c934457057f`), flashed at `0x10000`, and read-back verified successfully. The Device Operator subsequently reported restored Wi-Fi and retained ADMIN credentials; browser and physical acceptance were not independently repeated in this session. |

## Current objective

Next exact action: create the clean v2.7.10 source commit and release tag,
build the tagged image, OTA-install it on the authorized test unit, verify its
reported version and service boundaries, then publish the tag and matching
firmware/checksum assets.

## Read only when needed

| Need | Document |
| --- | --- |
| Active releases and branch scope | [ESP32_DEVELOPMENT_PLAN.md](ESP32_DEVELOPMENT_PLAN.md) |
| v2.7.10 status UI implementation and acceptance | [ESP32_V2_7_10_STATUS_UI_POLISH_SPEC.md](ESP32_V2_7_10_STATUS_UI_POLISH_SPEC.md) |
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
