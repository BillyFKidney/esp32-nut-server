# ESP32-NUT v2.8.11 release evidence

- Source merge: `945c5ce6c51a3dbb0f0789acd85f3a4bd26ed828`
- Annotated tag: `v2.8.11`
- GitHub release: <https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.8.11>
- Firmware asset: `nut-esp32s3-v2.8.11.bin` (1,359,760 bytes)
- SHA-256: `4f2ba22944c56ad28e9b96c66c799e6f0af51ab6028bf7f520d52156399ae344`

## Benefit

Malformed browser firmware uploads now fail predictably before they can begin
inactive-partition work, protecting the update path and providing an immediate
actionable error.

## Validation

- An independent scanner/fixer/reviewer loop accepted the new browser content-
  type gate. It preserves ADMIN/CSRF precedence, bearer-agent behavior, valid
  browser installation, existing error contracts, and OTA sequencing.
- A clean ESP-IDF v6.0.2 reconfigure/build from the exact tag was inspected
  with `esptool image_info`, confirming embedded `v2.8.11` metadata. The
  application is 1,359,760 bytes and leaves 59% app-slot headroom.
- The artifact OTA-installed on Agent Tests with explicit `HTTP 200` and
  `installed` response. The live browser route rejected missing, JSON,
  parameterized, and text content types with `415`; an invalid CSRF request
  returned `403`; the device remained at `v2.8.11` with update result
  `installed`. Proxy-pinned authenticated ADMIN acceptance passed; HTTPS `443`
  and read-only NUT `3493` responded; `8080` was refused; a raw NUT `LIST VAR`
  poll returned 57 variables with `ups.status` `OL`. Garage was not modified.
