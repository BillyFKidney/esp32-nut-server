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

## Planned acceptance

- Exact tag `v2.9.5` clean-builds under ESP-IDF v6.0.2.
- Certificate-pinned OTA installs the exact artifact on 3Dprinter.
- The post-reboot full NUT poll, HTTPS `443`, read-only NUT `3493`, and refused
  `8080` confirm normal USB polling and service recovery.
