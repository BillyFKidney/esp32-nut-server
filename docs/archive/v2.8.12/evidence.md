# ESP32-NUT v2.8.12 release evidence

- Source merge: `a7da9970ab3b27cbebb449ff2be1820624f1b413`
- Annotated tag: `v2.8.12`
- GitHub release: <https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.8.12>
- Firmware asset: `nut-esp32s3-v2.8.12.bin` (1,359,328 bytes)
- SHA-256: `c25965c3da80d6fbb08be467167e0808482b2a39b08b95453bf433830e0f386b`

## Benefit

API and diagnostic token issuance now share one fixed internal workflow,
reducing the risk that future security maintenance makes two sensitive paths
drift while preserving each credential family's separate authority.

## Validation

- Independent scan/fixer/reviewer accepted the private extraction, including
  fixed creator/scope pairing, exact response/error contracts, ADMIN/CSRF,
  time gating, one-time plaintext responses, and zeroization.
- A clean ESP-IDF v6.0.2 build from the exact tag was inspected with
  `esptool image_info`, confirming embedded `v2.8.12` metadata. The 1,359,328-
  byte application leaves 59% app-slot headroom.
- The artifact OTA-installed on Agent Tests with explicit `HTTP 200` and
  `installed` response. Certificate-pinned temporary API and diagnostic token
  lifecycle checks passed: creation, metadata isolation, duplicate rejection,
  cross-scope rejection, deletion, and revocation. Generated tokens were never
  displayed and both were revoked. Authenticated ADMIN acceptance passed;
  HTTPS `443` and read-only NUT `3493` responded; `8080` was refused; a raw
  NUT `LIST VAR` poll returned 57 variables with `ups.status` `OL`. Garage was
  not modified.
