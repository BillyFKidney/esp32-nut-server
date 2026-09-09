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

- **Observed (pre-tag):** The repaired
  `esp32nut-3dprinter.28670avenidacondesa.com` proxy resolves to
  `192.168.40.10`, presents a normally trusted certificate, returns HTTPS 200
  for the sign-in page, rejects unauthenticated `/api/v1/status` with 401, and
  accepts the scoped diagnostic-token Agent-status request with HTTP 200.
- **Observed (pre-tag):** The Project Maintainer's Chrome dashboard evidence
  shows the repaired FQDN serving the authenticated 3Dprinter UI. The installed
  development candidate reports connected Wi-Fi at `192.168.40.88`, NUT health
  on TCP 3493, and UPS status `OL`.
- **Observed (pre-tag):** Direct checks against `192.168.40.88` found HTTPS
  `443` and read-only NUT `3493` open, retired `8080` refused, fingerprint-pinned
  diagnostic Agent status healthy, and a complete 57-variable NUT poll with
  `ups.status OL`.
- **Pending:** Certificate-pinned scoped OTA of the exact tagged artifact to
  the authorized 3Dprinter Agent Tests unit, followed by the same FQDN and
  direct post-reboot acceptance checks.

## Release closeout

- **Pending:** Annotated `v2.8.1` tag, exact-tag rebuild, versioned firmware
  asset, matching SHA-256 sidecar, GitHub release URL, and verified published
  artifact checksums.
