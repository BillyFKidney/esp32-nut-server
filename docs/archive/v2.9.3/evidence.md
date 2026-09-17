# ESP32-NUT v2.9.3 release evidence

## Scope

The retained management-log ring now stores 50 `ManagementLogSnapshotEntry`
records in PSRAM (10,800 bytes at the ESP32-S3 ABI), instead of 24 static
internal-SRAM records (5,184 bytes). The hot 256-byte partial line, ring
indices, and lock remain internal. A FreeRTOS mutex replaces the prior
`portMUX`, so no PSRAM access occurs in a spinlock critical section. The
six-entry status window and ADMIN/CSRF boundaries remain unchanged; the
full-log snapshot/retrieval is explicitly allowed to grow with retention.

PSRAM allocation is strict. If it cannot initialize, log capture is disabled
and the existing status/full-log routes return empty log arrays rather than
silently consuming internal SRAM or failing management requests.

## Candidate acceptance

- Clean candidate `90b5bc4a8` built with ESP-IDF v6.0.2. Its linked DIRAM use
  was 127,843 bytes, 5,184 bytes below v2.9.2, matching removal of the static
  24-entry ring.
- Certificate-pinned scoped OTA installed the candidate on 3Dprinter. Its
  authenticated ADMIN console passed the 50-entry full-log capacity contract
  and unchanged six-entry status-log contract; diagnostics reported NUT `ok`,
  UPS `OL`, and the expected PSRAM allocation.
- A bounded three-client simultaneous console soak had one complete success;
  two requests failed while loading the ADMIN page before the log route, at
  the appliance HTTPS/session concurrency limit. A subsequent normal
  single-client ADMIN/log check and NUT/service acceptance passed.

## Exact-tag acceptance

- Annotated tag `v2.9.3` resolves to `3c64d3188`.
- The exact ESP-IDF v6.0.2 build embedded `v2.9.3`, produced a 1,362,656-byte
  image with 59% app-slot headroom, and SHA-256
  `a1cb117f76c825bcdc368ece1d4989e204d60e9e4c3a82d5c0b910f1593ca4c7`.
- Certificate-pinned scoped OTA installed the exact image on 3Dprinter in
  `app1`, preserving `app0` as rollback. The authenticated ADMIN console
  passed the 50-entry full-log capacity and six-entry status-window contracts.
- Post-reboot diagnostics report `v2.9.3`, update `installed`, PSRAM
  available, healthy NUT, and UPS `OL`. HTTPS `443` and read-only NUT `3493`
  responded, `8080` was refused, and read-only `LIST VAR` returned 57 values.
