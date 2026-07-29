# 2026-07-29 target candidate smoke validation

This record preserves the completed target evidence for the local integration
candidate. It is historical evidence, not an active branch-state document.

## Candidate identity

- **Branch at build time:** `candidate/wifi-provisioning-web-header-limit`
- **Source commit:** `0f008c6efe6e809f6d800b2406f0febd542c84d3`
- **Firmware-reported version after installation:** `v2.7.0-22-g0f008c6ef`
- **Application image SHA-256:**
  `32b71e34fe8812236ad54ae94ca8d5069ec8cf1cfccdaa2326c8d1dc393cac15`
- **Build environment:** ESP-IDF v6.0.2, target `esp32s3`

## Observed

- The Project Maintainer selected the local application image in the
  authenticated **Update Firmware** panel and approved installation.
- The browser check response stated that the firmware image was verified. At
  that time the response did not identify the image version; that usability gap
  is the reason for the subsequent `feature/ota-check-image-identity` slice.
- The device rebooted after installation. The dashboard reported firmware
  `v2.7.0-22-g0f008c6ef`, uptime `27s`, and last update `installed`.
- The Project Maintainer confirmed that the existing ADMIN password continued
  to work after the reboot.
- Network-only checks through the current management path returned HTTPS root
  status `200`; TCP `443` and TCP `3493` accepted connections; retired TCP
  `8080` was refused.

## Inferred

- The dashboard-reported identifier matches the candidate's Git-derived build
  identity, so the browser OTA installed the intended candidate rather than a
  stale image.
- The reboot and successful ADMIN authentication provide smoke evidence that
  the combined header-limit fix and Wi-Fi-provisioning-web extraction did not
  break the configured management path.

## Not tested

- The four temporary captive-portal routes were not exercised. Testing them
  would require deliberately entering recovery/fallback mode and is a separate
  approved acceptance session.
- A standard authenticated NUT client was not available in the local tool
  environment. Raw unauthenticated NUT probes returned `ERR ACCESS-DENIED`,
  which confirms neither a NUT regression nor live UPS values. TCP `3493`
  reachability alone was observed.
- No rollback, factory reset, direct serial recovery, or repeated OTA cycle was
  performed in this smoke session.

## Serial-flash note

An attempted direct application-slot serial write through the presently
enumerated USB interface failed during the first flash write with a transport
format error. It did not erase NVS, OTA metadata, or filesystem partitions; the
board remained reachable and the normal browser OTA path succeeded afterward.
Use the authenticated OTA path for normal development updates and treat direct
serial recovery as a separately scoped troubleshooting task.
