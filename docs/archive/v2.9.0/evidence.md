# ESP32-NUT v2.9.0 release evidence

## Scope

This release explicitly moves the two bounded management bulk-response
allocations to PSRAM: the 7,000-byte status JSON response and the 24-entry
full-log snapshot. It does not move task stacks, locks, session/auth state,
USB/DMA buffers, or OTA storage.

## Candidate acceptance

- Commit `32a8176a6` clean-built with ESP-IDF v6.0.2 and `idf.py size`.
  The image is 1,359,285 bytes with 59% app-slot headroom and SHA-256
  `0df9aeb3fa52b3b8c9d13b7fac56dab6c240ba3bbecccbe4f91164e6a6e8dc73`.
- Its certificate-pinned scoped OTA installed on 3Dprinter in `app1`.
- Post-reboot authenticated status/full-log, unauthenticated full-log, and
  invalid-CSRF acceptance passed. HTTPS `443` and NUT `3493` responded, `8080`
  was refused, and the NUT service recovered to UPS `OL`.
- The post-reboot idle status sample reported 115,027 free internal bytes and
  8,365,996 free PSRAM bytes. See `ESP32_MEMORY_USAGE.md` for the qualified
  baseline comparison.

## Exact-tag acceptance

- Annotated tag `v2.9.0` resolves to `0c02bd4a8`.
- The exact ESP-IDF v6.0.2 build embedded `v2.9.0`, produced a
  1,359,285-byte image with 59% app-slot headroom, and SHA-256
  `a116275bbb0b24cacf1155a20e8444a7568f47f8789a7168c730e7858e375f0c`.
- Certificate-pinned scoped OTA installed that exact image on 3Dprinter in
  `app0`. Post-reboot diagnostics report `v2.9.0`, update `installed`, healthy
  NUT, and UPS `OL`; HTTPS `443` and NUT `3493` respond while `8080` is refused.
