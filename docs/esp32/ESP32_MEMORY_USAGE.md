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
2026-09-17. Runtime samples taken before the `v2.9.0` candidate OTA reported
114,503 bytes free internal and 8,365,988 bytes free PSRAM. The reported
minimum-free value is a mixed-capability value, not an internal-heap minimum.
Task high-water marks remain pending dedicated instrumentation.

## `v2.9.0` candidate — PSRAM management response buffers

The candidate explicitly allocates the 7,000-byte status response and the
full-log route snapshot (24 entries, about 5.2 KiB) with
`MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT`. The exact committed candidate
`32a8176a6` was built with ESP-IDF v6.0.2 as a 1,359,285-byte image:

| Measurement | `v2.8.18` baseline | `v2.9.0` candidate | Change |
| --- | ---: | ---: | ---: |
| Flash code | 926,690 | 926,674 | -16 bytes |
| Flash data | 313,340 | 313,340 | 0 bytes |
| DIRAM | 132,931 | 132,931 | 0 bytes |
| Image | 1,359,301 | 1,359,285 | -16 bytes |
| Runtime free internal (post-reboot idle status sample) | 114,503 | 115,027 | +524 bytes |
| Runtime free PSRAM (post-reboot idle status sample) | 8,365,988 | 8,365,996 | +8 bytes |

The runtime samples were taken at different uptimes and do not measure the
transient buffers, because the status payload samples heap before allocating
its response. They therefore prove PSRAM availability and normal recovery, not
a precise saved-byte result. The explicit capability allocations prove the
two bounded live buffers now request PSRAM; their expected concurrent internal
heap relief is 12,184 bytes plus allocator overhead.

## `v2.9.1` candidate — PSRAM Wi-Fi scan records

Both scan paths now share a PSRAM-first allocator for their driver-populated
`wifi_ap_record_t` array. The ESP32-S3 target ABI is 92 bytes per record and
each path retains at most 20 records, so the maximum transient allocation is
1,840 bytes. The allocator requests `MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT` and
uses ordinary internal allocation only when that request cannot be satisfied.

Commit `f89d1cc37` clean-built with ESP-IDF v6.0.2 as a 1,359,309-byte image:

| Measurement | `v2.9.0` candidate | `v2.9.1` candidate | Change |
| --- | ---: | ---: | ---: |
| Flash code | 926,674 | 926,698 | +24 bytes |
| Flash data | 313,340 | 313,340 | 0 bytes |
| DIRAM | 132,931 | 132,931 | 0 bytes |
| Image | 1,359,285 | 1,359,309 | +24 bytes |
| Runtime free internal (post-reboot idle sample) | 115,027 | 115,119 | +92 bytes |
| Runtime free PSRAM (post-reboot idle sample) | 8,365,996 | 8,365,996 | 0 bytes |

The idle samples occur after the transient scan array is freed, so they do not
measure its 1,840-byte concurrent internal-heap relief. The certificate-pinned
target scan returned HTTP 200 through `esp_wifi_scan_get_ap_records()` while
PSRAM was available, proving the driver accepted the selected external-memory
destination. The portal path uses the same allocator but was not independently
activated on the connected appliance because doing so would require disruptive
Wi-Fi recovery.

## `v2.9.2` exact tag — browser staged PSRAM OTA

The browser OTA check allocates exactly the request image length with
`MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT`, with no internal fallback. It validates
there and retains the allocation for at most ten minutes. The explicit install
streams the checked PSRAM image to the inactive slot in 4,096-byte chunks, then
zeroizes and frees the stage. The retained Agent route uses a 4,096-byte
PSRAM-first receive buffer with an internal fallback so its existing update
availability contract remains intact.

| Measurement | `v2.9.1` exact tag | `v2.9.2` exact tag | Change |
| --- | ---: | ---: | ---: |
| Flash code | 926,698 | 929,026 | +2,328 bytes |
| Flash data | 313,340 | 313,964 | +624 bytes |
| DIRAM | 132,931 | 133,027 | +96 bytes |
| Image | 1,359,309 | 1,362,384 | +3,075 bytes |
| Runtime free internal (post-reboot idle sample) | 115,119 | 114,875 | -244 bytes |
| Runtime free PSRAM (post-reboot idle sample) | 8,365,996 | 8,365,992 | -4 bytes |

The exact-tag device stage/install test showed PSRAM return to its post-reboot
baseline after installation. A pre-tag candidate stage reduced free PSRAM from
8,366,468 to 6,990,132 bytes while retaining its 1,362,384-byte image; the
remaining difference includes allocator behavior and contemporaneous runtime
state, so it is evidence of placement rather than an exact overhead measure.
The exact-tag idle samples are at different uptimes and are not a precise
internal-SRAM saving measurement. They demonstrate that staging is transient
and that idle PSRAM availability/recovery is preserved.

## `v2.9.3` exact tag — 50-entry PSRAM management-log ring

The target ABI makes each `ManagementLogSnapshotEntry` 216 bytes. The ring is
now `50 * 216 = 10,800` bytes in strict PSRAM; the prior static internal ring
was `24 * 216 = 5,184` bytes. Its 256-byte partial-line buffer, lock, and
indices intentionally remain internal because capture is frequent. The
full-log route’s separate PSRAM snapshot grows to 10,800 bytes only during a
retrieval; the six-entry status copy remains a 1,296-byte stack object.

| Measurement | `v2.9.2` exact tag | `v2.9.3` exact tag | Change |
| --- | ---: | ---: | ---: |
| Flash code | 929,026 | 929,238 | +212 bytes |
| Flash data | 313,964 | 314,044 | +80 bytes |
| DIRAM | 133,027 | 127,843 | -5,184 bytes |
| Image | 1,362,384 | 1,362,656 | +272 bytes |
| Runtime free internal (post-reboot idle sample) | 114,875 | 118,383 | +3,508 bytes |
| Runtime free PSRAM (post-reboot idle sample) | 8,365,992 | 8,355,372 | -10,620 bytes |

The linked DIRAM delta exactly accounts for the previous static ring. The
runtime samples are at different uptimes, so they do not precisely quantify
allocator overhead; together with the strict capability allocation they prove
the intended placement and retained-capacity increase. A 100-entry ring would
need 21,600 PSRAM bytes and is deferred pending a dedicated soak.

## `v2.9.4` audit — session/auth state and locks retained internally

No PSRAM allocation is appropriate for the remaining persistent session/auth
state. Target object evidence shows a 144-byte `ManagementSession`, two 8-byte
`portMUX_TYPE` objects, and 12 bytes of login cooldown/failure state. These
are hot and security-sensitive; moving them would trade a negligible internal
saving for slower external access and a larger secret-memory exposure. Token
records are NVS-persisted and only exist in bounded, zeroized operation-local
storage, so they are not persistent SRAM migration candidates. This release
must append exact-tag linked/runtime values, expected to differ only in its
version metadata, after its target acceptance.

Exact `v2.9.4` evidence: Flash code 929,238 bytes, Flash data 314,044 bytes,
DIRAM 127,843 bytes, and 1,362,656-byte image—unchanged from v2.9.3. The
post-reboot sample reported 119,999 bytes free internal and 8,354,992 bytes
free PSRAM. Differences from v2.9.3 are runtime/uptime variation, not a
placement change; no auth or lock allocation moved.
