# v2.7.1 release validation record

## Publication

`v2.7.1` was published from the accepted merge commit after an ESP-IDF v6.0.2
ESP32-S3 build. The release contains the application image and SHA-256 checksum.

## Target observation

After publication, the Project Maintainer installed the `v2.7.1` image and
reported that all requested tests passed. The device reported firmware
`v2.7.1`, was connected to Wi-Fi, and was responsive through the management
console.

## Included acceptance evidence

- Management route-family extraction preserved its protected boundaries.
- ADMIN password rotation followed by sign-in with the new password passed.
- Routine ESP-IDF HTTPS handshake messages no longer filled the browser runtime
  log snapshot.
- Earlier target checks covered Wi-Fi scan/configuration, token issuance and
  revocation, time behavior, authenticated management, and fresh read-only UPS
  data.

## Notes

NUT may warn that its FAT-backed `upsd.users` file is world-readable. In this
read-only configuration the file contains no NUT users or password material;
the warning is retained as an upstream/runtime limitation, not treated as a
release blocker.
