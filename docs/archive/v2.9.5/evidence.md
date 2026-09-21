# ESP32-NUT v2.9.5 HID descriptor placement audit

## Scope

This release closes the final candidate in the PSRAM placement program with a
no-move decision. `MAX_REPORT` is 500 and the target `HIDData_t` ABI is 84
bytes, making the parser's maximum temporary allocation 42,000 bytes. The
array is not temporary in practice: after parsing, `hid_ups_walk()` stores
pointers to its entries in `hid_info_t::hiddata`. Its quick and full updates
then pass those pointers to `HIDGetDataValue()` for recurring USB report reads.

The ordinary allocator may use PSRAM for this allocation because it exceeds the
internal-preference threshold. Making it strict PSRAM would instead require
external memory for long-lived, frequently dereferenced USB polling metadata.
That is counter to the placement policy for hot/timing-sensitive objects, so no
source allocation changes are made. Parser scratch and USB/DMA transfer buffers
remain internal. Descriptor parsing, reconnect, supported-device behavior, and
read-only UPS behavior are unchanged.

## Acceptance

- Exact tag `v2.9.5` resolves to `477235bc79480321ef3bb9582bb28cffa940baf6`.
  A clean ESP-IDF v6.0.2 reconfigure/build embeds `v2.9.5` and produces the
  1,362,656-byte artifact with SHA-256
  `104c75ee69849c65f440dc30c391a00444314810585458ad2dab0866f1c6c4b1`.
- Certificate-pinned Agent OTA installed that exact artifact on 3Dprinter. The
  post-reboot diagnostic reports `v2.9.5` from `app0`, `app1` as next slot,
  `installed`, and 120,011 bytes free internal / 8,354,984 bytes free PSRAM.
- Authenticated console acceptance passed its 50-entry full-log, six-entry
  status-log, bounded Wi-Fi scan, unauthenticated rejection, and CSRF-rejection
  checks. Read-only NUT returned 57 variables with UPS `OL`; HTTPS `443` and
  NUT `3493` responded and retired `8080` was refused.
