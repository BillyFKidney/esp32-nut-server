# ESP32-NUT v2.8.10 release evidence

- Source merge: `d8b7d6cba3fd3ac5dc4cca80955327ea29951737`
- Annotated tag: `v2.8.10`
- GitHub release: <https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.8.10>
- Firmware asset: `nut-esp32s3-v2.8.10.bin` (1,359,664 bytes)
- SHA-256: `51e5210df5ac63ccdaf1d6d5dd4b16eb1cadec395d9bee37de8f289410148a30`

## Benefit

Administrators can use valid display names containing quotes or backslashes
without breaking the JSON returned to the management UI or API clients.

## Validation

- An independent scanner/fixer/reviewer loop accepted the bounded JSON
  serialization change. It preserves device-name validation, persistence,
  field names/order, hostname application, response zeroization, and
  ADMIN/CSRF behavior.
- A clean ESP-IDF v6.0.2 reconfigure/build from the exact tag was inspected
  with `esptool image_info`, confirming embedded `v2.8.10` metadata. The
  application is 1,359,664 bytes and leaves 59% app-slot headroom.
- The artifact OTA-installed on Agent Tests. The live status reports firmware
  `v2.8.10`, running `app1`, update result `installed`, and healthy NUT state.
  Proxy-pinned authenticated ADMIN acceptance passed; HTTPS `443` and
  read-only NUT `3493` responded; `8080` was refused; a raw NUT `LIST VAR`
  poll returned 57 variables with `ups.status` `OL`. Garage was not modified.
