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
- Completed clean exact-tag ESP-IDF v6.0.2 build with 59% app-slot headroom.
- Certificate-pinned scoped OTA installed the exact artifact on 3Dprinter.
- Post-reboot acceptance confirmed firmware `v2.8.18`, HTTPS `443`, read-only
  NUT `3493`, refused `8080`, update `installed`, healthy NUT, and UPS `OL`.

## Artifact

`nut-esp32s3-v2.8.18.bin` — SHA-256
`faa4409567e39b700714bd092226cf9390d132b3ff99fa08340a44144f1a5159`
