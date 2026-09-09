# v2.8.3 release evidence

## Scope

The v2.8.3 review release extracts the duplicated OTA and diagnostic token-store
lifecycle into a private store. It preserves the management NVS namespace and
blob layout, verifier-only token storage, scope isolation, and existing
ADMIN/CSRF route contracts.

## Release source and artifact

- Source: annotated tag `v2.8.3` at `0d9e433a5d601abf77a67ba3ff1fad43ca5a09fc`.
- Build: clean exact-tag ESP-IDF v6.0.2 reconfigure/build and size check.
- Artifact: `nut-esp32s3-v2.8.3.bin`, 1,359,808 bytes.
- SHA-256: `3ba9ba14e38af156c29566cf635e4c0087a2699dbbf1b861b140bb1daeb186ac`.
- Publication: [GitHub release v2.8.3](https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.8.3), including the firmware and SHA-256 sidecar.

## Acceptance

- Fresh Agent Tests variables passed redacted presence and format validation.
- Temporary live tokens passed create, metadata-only list, authorization-scope
  isolation, delete, and post-deletion revocation checks.
- The exact tagged artifact installed through the scoped Agent OTA route.
- After reboot, diagnostics reported firmware `v2.8.3` and update `installed`.
- HTTPS `443` and read-only NUT `3493` were reachable; retired `8080` was refused.
- A complete NUT `LIST VAR` poll returned 57 variables and `ups.status` `OL`.

## Not tested

The Garage device was not modified. Its registered 1Password local mount was
unavailable on disk and the provider's per-device mount limit prevented a fresh
mount during release closeout.
