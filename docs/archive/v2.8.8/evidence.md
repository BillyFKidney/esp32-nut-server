# ESP32-NUT v2.8.8 release evidence

- Source merge: `2126378f10703636485f09ef34c6ffaacce889cd`
- Annotated tag: `v2.8.8`
- GitHub release: <https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.8.8>
- Firmware asset: `nut-esp32s3-v2.8.8.bin` (1,359,568 bytes)
- SHA-256: `269cfaa42bca22c01183a3a0e77cc2dc9dd5bd58433f6ec8b8665052e429c0f6`

The review extracted one private decoder shared by management form field names
and values. Independent review confirmed the exact existing behavior for
malformed/incomplete percent escapes, invalid hexadecimal pairs, embedded NUL,
plus-to-space conversion, truncation, duplicate ordering, separators, and
empty fields. No API, route, authorization, or storage behavior changed.

The exact tag passed a clean ESP-IDF v6.0.2 build with 59% app-slot headroom.
Both 1Password environments passed redacted validation before device work;
only Agent Tests was modified. The checksum-verified image OTA-installed, then
pinned diagnostics reported `v2.8.8` and `installed`. Authenticated ADMIN
form/log checks, 443/3493 availability, refused 8080, and a full 57-variable
`cyberpower` NUT poll with `ups.status` `OL` passed. Garage was not modified.
