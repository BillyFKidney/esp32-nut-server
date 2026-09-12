# ESP32-NUT v2.8.14 release evidence

- Source release commit: `e5248ca34`
- Annotated tag: `v2.8.14`
- GitHub release: <https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.8.14>
- Firmware asset: `nut-esp32s3-v2.8.14.bin` (1,359,345 bytes)
- SHA-256: `c29012b2af9490a06bb283739006f110c846aa8940323ae42cfd03c149038fcb`

## Benefit

The setup network shuts down more reliably when its DNS worker is slow to
exit. This prevents a rare teardown race from destabilizing Wi-Fi recovery,
without changing normal DNS, portal, provisioning, or station behavior.

## Validation

- The completed scanner/fixer/reviewer loop accepted the narrow lifetime fix;
  independent review preserved DNS responses, the one-second graceful-stop
  behavior, portal integration, and normal station behavior.
- A clean ESP-IDF v6.0.2 build from the exact tag embedded `v2.8.14`, produced
  the recorded artifact and SHA-256, and left 59% application-slot headroom.
- Scoped certificate-pinned OTA installed successfully on 3Dprinter. After
  reboot, diagnostics reported `v2.8.14`, `installed`, normal Wi-Fi recovery,
  and healthy NUT data with UPS status `OL`.
- Holding BOOT for the Wi-Fi-only recovery interval started the open
  `ESP32-NUT-60D4E5` setup AP. A DNS query for `example.com` returned
  `192.168.4.1`; root, status, network-list, and unknown-route redirect
  acceptance passed.
- The authorized IoT credential submitted through the portal returned 200 and
  restored 3Dprinter to `ClubHouse_IoT`. HTTPS `443` and read-only NUT `3493`
  responded, `8080` was refused, and the complete raw NUT poll reported UPS
  status `OL`.
