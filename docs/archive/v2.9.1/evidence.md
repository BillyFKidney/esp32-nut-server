# ESP32-NUT v2.9.1 release evidence

## Scope

This release moves each bounded, transient Wi-Fi driver scan-record array to
PSRAM first: the authenticated ADMIN scan and fallback captive-portal scan
share one allocator. Each retains no more than 20 target-ABI records (1,840
bytes). Internal SRAM remains a fallback only if PSRAM allocation is
unavailable; task stacks, locks, Wi-Fi state, and DMA buffers remain internal.

## Candidate acceptance

- Commit `f89d1cc37` clean-reconfigured and built with ESP-IDF v6.0.2. Its
  embedded version is `v2.9.0-3-gf89d1cc37`; the 1,359,309-byte image has 59%
  app-slot headroom and SHA-256
  `d40f887384f8d30496372cc0bd0178ce88fc998596591f1ce5328d1192e58247`.
- Certificate-pinned scoped OTA installed the candidate on 3Dprinter in
  `app1`.
- The authenticated ADMIN scan returned HTTP 200 with a valid bounded network
  array, demonstrating that `esp_wifi_scan_get_ap_records()` accepts the
  PSRAM-selected driver destination. The portal path shares that allocator but
  was not independently activated because its safe trigger is unavailable on
  the connected appliance.
- After reboot, diagnostics reported the candidate version, update `installed`,
  PSRAM available, healthy NUT, and UPS `OL`. HTTPS `443` and read-only NUT
  `3493` remained available; `8080` remained refused.

## Exact-tag acceptance

- Annotated tag `v2.9.1` resolves to `e37f9f626`.
- The exact ESP-IDF v6.0.2 build embedded `v2.9.1`, produced a 1,359,309-byte
  image with 59% app-slot headroom, and SHA-256
  `3d0f0478bb6ff9445cd42341d62f6285fd1609f0bcf227223bb3c1598935268d`.
- Certificate-pinned scoped OTA installed that exact image on 3Dprinter in
  `app0`. Post-reboot diagnostics report `v2.9.1`, update `installed`, PSRAM
  available, healthy NUT, and UPS `OL`.
- The authenticated ADMIN scan passed again with a valid bounded network array.
  HTTPS `443` and read-only NUT `3493` responded, `8080` was refused, and a
  read-only `LIST VAR` poll returned 57 variables.
