# ESP32-NUT v2.8.9 release evidence

- Source merge: `96573649ee75267956062a275dce87e82559137b`
- Annotated tag: `v2.8.9`
- GitHub release: <https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.8.9>
- Firmware asset: `nut-esp32s3-v2.8.9.bin` (1,359,552 bytes)
- SHA-256: `1daa91ba7835136af4151335ff2c01151f90553dab91bdcf47a49314ab63f49d`

## Benefit

Credential-format rules now have one source of truth. This reduces the risk
that future security maintenance makes stored-record validation and password
verification disagree.

## Validation

- Independent scan and security-focused review accepted the predicate reuse
  while preserving schema, migration, iteration bounds, PBKDF2, constant-time
  comparison, handle closure, and zeroization.
- An initial incremental artifact retained v2.8.8 metadata and was not
  published. A clean ESP-IDF v6.0.2 reconfigure/build was inspected with
  `esptool image_info`, confirming embedded v2.8.9 metadata before OTA.
- The corrected artifact OTA-installed on Agent Tests. Pinned diagnostics
  report v2.8.9 and `installed`; ADMIN/CSRF contracts, HTTPS `443`, read-only
  NUT `3493`, refused `8080`, and a full 57-variable `cyberpower` poll with
  `ups.status` `OL` passed. Garage was not modified.
