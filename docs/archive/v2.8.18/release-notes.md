## Benefit

v2.8.18 makes firmware updates more reliable under low-memory conditions. If
the normal delayed reboot task cannot be created after a verified image is
selected, the device immediately restarts rather than reporting success and
remaining on the old firmware.

## What changed

The OTA core now immediately restarts only when the existing success response
has been queued but FreeRTOS cannot schedule the normal one-second reboot task.
Normal delayed restart, OTA responses, NVS result semantics, image validation,
partition selection, authorization, and check-only behavior are preserved.

## Verification

- Completed independent scanner, fixer, and reviewer loop: clean.
- Completed clean ESP-IDF v6.0.2 candidate build and certificate-pinned scoped
  OTA on 3Dprinter.
- Post-reboot candidate acceptance confirmed HTTPS `443`, read-only NUT
  `3493`, refused `8080`, update `installed`, healthy NUT, and UPS `OL`.

## Artifact

Final versioned binary and SHA-256 are recorded after the exact-tag build.
