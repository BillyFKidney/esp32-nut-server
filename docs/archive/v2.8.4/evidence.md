# v2.8.4 release evidence

## Scope

The v2.8.4 review release extracts the OTA receive/write/verify/abort lifecycle
from request orchestration and names the existing reboot-task stack size and
priority. It preserves the locked authorization, response, NVS-result,
partition-selection, abort, and reboot contracts.

## Release source and artifact

- Source: annotated tag `v2.8.4` at `d57a08bf9d280748c6a4de9dcc68e55c4c945188`.
- Build: clean exact-tag ESP-IDF v6.0.2 reconfigure/build and size check.
- Artifact: `nut-esp32s3-v2.8.4.bin`, 1,359,824 bytes.
- SHA-256: `3abea36c7ad62a97033edb7277c913da2f83eccc4847f84f41bd84c748ce5987`.
- Publication: [GitHub release v2.8.4](https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.8.4), including the firmware and SHA-256 sidecar.

## Acceptance

- Both Agent Tests and Garage 1Password variable inventories passed redacted
  presence and format validation; Garage was not modified.
- Independent focused scan and source review passed against the locked OTA
  route contract.
- The exact tagged artifact installed through the scoped Agent OTA route.
- After reboot, diagnostics reported firmware `v2.8.4` and update `installed`.
- HTTPS `443` and read-only NUT `3493` were reachable; retired `8080` was refused.
- A complete NUT `LIST VAR` poll returned 57 variables and `ups.status` `OL`.
