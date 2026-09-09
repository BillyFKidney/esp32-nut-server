# v2.8.1 code-quality maintenance evidence

## Scope

- **Observed:** `v2.8.1` is a maintenance release for narrowly reviewed
  code-quality improvements. It adds no product features and preserves the
  released HTTPS, ADMIN/CSRF, scoped-token, read-only NUT, and refused-8080
  contracts.
- **Required:** Release evidence must identify the exact tagged source commit,
  clean ESP-IDF v6.0.2 build identity, artifact size, SHA-256, and app-slot
  headroom.

## Acceptance record

- **Observed:** The exact annotated `v2.8.1` tag at `c6bf2d1be` clean-built
  with ESP-IDF v6.0.2. `nut-esp32s3.bin` is 1,362,080 bytes, has SHA-256
  `ebb20c6a047c42498e2d4d382b44bc91e2a8e3d4413d44999eab8f6fc0fc11f3`, and
  retains 59% app-partition headroom.
- **Observed:** Certificate-pinned scoped OTA installed that exact artifact on
  the authorized 3Dprinter Agent Tests unit at `192.168.40.88`. After reboot,
  authenticated Agent status reports firmware `v2.8.1`, running slot `app0`,
  last update `installed`, and healthy Wi-Fi/time synchronization.
- **Observed:** The post-reboot direct acceptance checks found read-only NUT
  `3493` healthy, a full 57-variable poll with `ups.status OL`, and retired
  `8080` refused.
- **Observed:** The repaired
  `esp32nut-3dprinter.28670avenidacondesa.com` proxy resolves to
  `192.168.40.10`, presents a normally trusted certificate, returns HTTPS 200
  for the sign-in page, rejects unauthenticated `/api/v1/status` with 401, and
  accepts the scoped diagnostic-token Agent-status request for firmware
  `v2.8.1` with HTTP 200.
- **Observed (pre-tag):** The Project Maintainer's Chrome dashboard evidence
  shows the repaired FQDN serving the authenticated 3Dprinter UI. The installed
  development candidate reports connected Wi-Fi at `192.168.40.88`, NUT health
  on TCP 3493, and UPS status `OL`.
- **Observed (pre-tag):** Direct checks against `192.168.40.88` found HTTPS
  `443` and read-only NUT `3493` open, retired `8080` refused, fingerprint-pinned
  diagnostic Agent status healthy, and a complete 57-variable NUT poll with
  `ups.status OL`.

## Release closeout

- **Complete:** The annotated `v2.8.1` tag identifies the exact tested source.
  The [GitHub release](https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.8.1)
  publishes `nut-esp32s3-v2.8.1.bin` and its matching SHA-256 sidecar. GitHub
  reports the firmware asset digest as
  `ebb20c6a047c42498e2d4d382b44bc91e2a8e3d4413d44999eab8f6fc0fc11f3`.
