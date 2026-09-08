# v2.8.0 candidate evidence

## State and scope

- **Observed:** Development branch `feature/optimization` began from `main`
  commit `517980b78523bfdcd3fef0c3a325475fd511b7ca`.
- **Observed:** v1 routes, payload shapes, ADMIN sessions, CSRF rules, bearer
  scopes, and the six-entry `/api/v1/status` `logs` field remain unchanged.
- **Observed:** Dashboard does not contain or render logs. Device Status uses
  `JSON.stringify(data, null, 2)` presentation while Copy JSON retains the
  exact response text. The dedicated Logs page lazily reads the existing
  ordered 24-entry `/api/v1/admin/logs` snapshot and provides Refresh, Copy,
  Download, empty, error, and clipboard-fallback states.
- **Observed:** The self-contained UI uses system fonts, CSS, native `meter`
  indicators, and vanilla JavaScript; it adds no remote or vendored runtime
  dependency.
- **Observed:** The redundant certificate/LAN-only header notice and ADMIN
  session footer notice were removed. Setup, sign-in, throttled sign-in, and
  ADMIN pages declare `/favicon.ico`; that route serves the repository's
  canonical 60x60 NUT PNG without authentication because it contains no
  device or session data.
- **Observed:** FreeRTOS is configured for both ESP32-S3 cores. USB host,
  discovery, HID, driver, and NUT tasks are intentionally pinned to core 0;
  management/network support tasks created without affinity remain available
  to the dual-core scheduler.
- **Decision:** No task affinity was changed in this UI/resource slice. Moving
  driver-owned work without stack, USB, Wi-Fi, NUT, watchdog, and rollback
  measurements would increase risk without demonstrating better UX. Lazy log
  fetching and streamed HTML remove browser-visible blocking and heap pressure
  without disturbing that architecture.

## Resource and build evidence

- **Observed:** The 48,089-byte flash-resident ADMIN page is sent in bounded
  chunks using a 768-byte stack buffer. The former 49,152-byte transient
  whole-page heap allocation is no longer present.
- **Observed:** The macOS embedded-JavaScript validator passed and now checks
  the streamed-page limit, Dashboard/Logs separation, pretty/raw JSON
  behavior, one lazy full-log request, and persistent header controls.
- **Observed:** ESP-IDF v6.0.2 `idf.py reconfigure && idf.py build` passed.
  `build/nut-esp32s3.bin` is 1,362,432 bytes; the 0x330000-byte app slots have
  59% free.
- **Observed:** `idf.py size` reports 929,806 bytes of flash code, 313,276
  bytes of flash data, and 132,899 of 341,760 DIRAM bytes used (38.89%).
- **Observed:** The installed dirty candidate SHA-256 is
  `323dc2516d21cc5a39b9cf6af8cab3864151acbd0b08c16ece12ed072f80ee03`.
- **Observed:** The certificate-pinned OTA helper initially encountered a TLS
  record-layer failure while writing 64 KiB chunks. Aligning the helper with
  the firmware's 4 KiB receive buffer produced HTTP 200 and a verified OTA.

## Device and contract acceptance

- **Observed:** The authorized `3Dprinter` Agent Tests unit installed the
  notice-removal and NUT-favicon dirty candidate in `app1`; it reports firmware
  `v2.8.0-evidence-dirty` and update state `installed`.
- **Observed:** A site power blink occurred during testing. At the Project
  Maintainer's direction, all device retry counts were reset and observations
  after the blink were treated as a fresh baseline. The blink explains the
  intervening reboot and is not attributed to the candidate.
- **Observed after the reset:** HTTPS `443` and read-only NUT `3493` respond;
  retired `8080` is refused. A complete NUT `LIST VAR cyberpower` transaction
  returned 57 variables, included `ups.status "OL"`, and ended with
  `OK Goodbye`.
- **Observed after the reset:** The credential-safe live ADMIN validator
  authenticated with a certificate pin, fetched the streamed page, confirmed
  both notices are absent, confirmed the NUT favicon returns HTTP 200 as a PNG,
  confirmed the unchanged six-entry status-log window and 24-entry ADMIN
  retention metadata, and confirmed unauthenticated `401` and invalid-CSRF
  `403`. The live favicon SHA-256 exactly matched `docs/images/nut-logo.png`:
  `85aa4a1ca59c51a065fb181c2b41c9a4303d1ac670ed2d6e07d374869ed0ba79`.
- **Observed after the favicon OTA:** HTTPS `443` and NUT `3493` respond,
  retired `8080` is refused, and a complete 57-variable NUT transaction ended
  cleanly with `ups.status "OL"`.
- **Observed:** Garage did not answer the refreshed post-blink diagnostic
  request. It was not OTA-updated or otherwise modified.

## Remaining acceptance and release state

- **Observed from Project Maintainer screenshots:** Chrome rendered the pushed
  candidate on desktop and in iPhone 16 Pro Max portrait/landscape and iPad Pro
  responsive emulation. The appliance header, single-row horizontally
  scrollable navigation, dashboard cards, full-width diagnostics, pretty JSON,
  dedicated Logs page, settings forms, API-token page, and OTA page were all
  visibly rendered. Navigation among those panels was exercised to capture the
  screenshots. The Project Maintainer's final Chrome check accepted the UI as
  great and accepted the remaining icon issue, closing the visual browser gate.
  The two screenshot console 404s were requests to external
  `c.1password.com/richicons/...` images for 1Password entries, not the ESP32
  `/favicon.ico`; they do not indicate a device favicon failure.
- **Not tested:** Clipboard success/denial fallback and downloaded file
  behavior still require explicit browser interaction evidence.
- **Not tested:** Garage post-update behavior; Garage was unavailable after the
  power blink and intentionally left untouched.
- **Release state:** The visual browser gate is complete. Clean build, tag,
  publication, and final artifact facts are intentionally not recorded here
  until those actions are authorized and completed. The dirty candidate and
  this evidence remain preserved on the feature branch.
