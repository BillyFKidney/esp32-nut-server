# ESP32-NUT v2.8.13 release evidence

- Source release commit: `1c9207c8a`
- Annotated tag: `v2.8.13`
- GitHub release: <https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.8.13>
- Firmware asset: `nut-esp32s3-v2.8.13.bin` (1,359,392 bytes)
- SHA-256: `8501786485bd5331fd86fd359f8bb3987575ac4d841b85731bbbc71cfeabe96c`

## Benefit

The fallback captive portal now has one private lifecycle owner. This makes
future Wi-Fi maintenance safer and easier to review without changing how the
device starts its setup AP, provides DNS, validates credentials, or recovers
normal station service.

## Validation

- The completed scanner/fixer/reviewer loop accepted the narrow extraction and
  preserved AP+STA/open-AP/DHCP-DNS behavior, synchronization ordering, task
  lifetime, cleanup, provisioning, and unrelated station behavior.
- A clean ESP-IDF v6.0.2 build from the exact tag embedded `v2.8.13`, produced
  the recorded artifact and SHA-256, and left 59% application-slot headroom.
- The scoped certificate-pinned OTA installed successfully on 3Dprinter into
  `app1`. Post-reboot diagnostic status reported `v2.8.13`, `installed`, and
  healthy NUT data with `ups.status` `OL`.
- Holding BOOT for the Wi-Fi-only recovery interval started the open
  `ESP32-NUT-60D4E5` setup AP. Its root, status, network-list, and unknown-path
  redirect routes passed; malformed provisioning input was rejected while the
  portal remained available.
- The authorized IoT credential submitted through the portal returned 200 and
  restored 3Dprinter to its normal station network. HTTPS `443` and read-only
  NUT `3493` responded, `8080` was refused, and raw NUT polling returned 57
  variables with `ups.status` `OL`.
