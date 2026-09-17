# ESP32-NUT memory-usage ledger

This ledger records reproducible memory evidence before and after each PSRAM
release. It distinguishes linked-image measurements from runtime heap evidence;
neither is inferred from the module's advertised capacity.

## Baseline — `v2.8.18` source (`4bc59b60228260a58fb5f1e83b58ce950a8b27e1`)

### Observed configuration and allocation policy

| Area | Current fact |
| --- | --- |
| PSRAM hardware | 8 MiB octal PSRAM at 80 MHz, boot initialized and memory tested (`CONFIG_SPIRAM=y`). |
| General heap policy | Ordinary allocations below 16 KiB prefer internal memory; a 32 KiB internal reserve is retained. Larger ordinary allocations may use PSRAM. No application code explicitly requests `MALLOC_CAP_SPIRAM`. |
| External BSS/rodata | Disabled. Static writable objects remain internal; code and `const` assets remain flash mapped. |
| Current exact linked usage | Clean ESP-IDF v6.0.2 rebuild: Flash code 926,690 bytes; Flash data 313,340 bytes; DIRAM 132,931 / 341,760 bytes (38.9%); IRAM 16,384 / 16,384 bytes; total image 1,359,301 bytes. |
| Current exact runtime heap usage | Pending a device baseline captured through `heap_caps_get_*`; no current runtime sample is stored in the checkout. |

### Source-accounted RAM residents

| Placement now | Item | Size / bound | Notes |
| --- | --- | --- | --- |
| Internal SRAM | Application task stacks | At least 60 KiB configured across NUT driver/server, management, USB, portal, DNS, recovery, restart, and OTA tasks | Stacks remain internal pending dedicated timing and high-water-mark evidence. |
| Internal heap | Status JSON response | 7,000 bytes per status request | Planned for explicit PSRAM in `v2.9.0`. |
| Internal heap | Full-log route snapshot | 24 `ManagementLogSnapshotEntry` values, about 5.2 KiB at the ESP32-S3 ABI | Planned for explicit PSRAM in `v2.9.0`. |
| Internal SRAM | Persistent management-log ring | 24 entries, about 5.2 KiB; pending chunk 256 bytes | Lock-protected; planned separately with synchronization redesign. |
| Internal heap | Wi-Fi scan records | Up to `20 * sizeof(wifi_ap_record_t)` per active portal or management scan | Exact target ABI size is measured in `v2.9.1`; planned for PSRAM with capability fallback. |
| Internal SRAM | OTA receive buffer | 4,096-byte stack object | Planned only with a recovery-safe PSRAM/internal fallback. |
| Internal SRAM | Session/auth state and locks | Session about 152 bytes plus `portMUX_TYPE` locks and counters | Hot, secret/authorization state; expected to remain internal after `v2.9.4` audit. |
| PSRAM by allocator policy | Initial HID descriptor array | `500 * sizeof(HIDData_t)`; about 42 KiB at the ESP32-S3 ABI | Current ordinary allocation crosses the 16 KiB threshold; placement is implicit and will be made contractual in `v2.9.5`. |

### Flash

The target has 16 MiB physical flash. Its two OTA application slots are each
3,342,336 bytes. The released `v2.8.18` artifact is 1,359,301 bytes (59% app
slot headroom). Management pages, portal HTML, embedded logo, code, and other
`const` data are flash-resident; they are not PSRAM migration candidates.

### Comparison baseline

No before/after delta is available until the first PSRAM release is built and
measured. Each release entry below must append linked flash/DIRAM results,
runtime internal/PSRAM heap totals and minima under its exercised workload,
and a signed direction-of-change statement rather than an inferred saving.

Build evidence: clean ESP-IDF v6.0.2 build and `idf.py size` completed on
2026-09-17. Runtime heap and task high-water-mark measurements remain pending
the first scoped device validation.
