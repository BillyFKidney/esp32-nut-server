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

- **Pending:** Certificate-pinned scoped OTA of the clean candidate to the
  authorized 3Dprinter Agent Tests unit.
- **Pending:** Repaired-proxy FQDN browser acceptance, including ADMIN login,
  status/log pages, unauthenticated rejection, and CSRF rejection.
- **Pending:** HTTPS `443`, read-only NUT `3493`, refused `8080`, authenticated
  Agent status, and a post-reboot full successful NUT poll.

## Release closeout

- **Pending:** Annotated `v2.8.1` tag, exact-tag rebuild, versioned firmware
  asset, matching SHA-256 sidecar, GitHub release URL, and verified published
  artifact checksums.
