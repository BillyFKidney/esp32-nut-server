# Wi-Fi scan code-review contract

## Scope

This `v2.8.Z` review slice addresses only the scan-lifecycle routine in
`src/wifi.c` and the unreachable duplicate button-release loop. It may extract
private helpers and restore the missing AP-list cleanup on the record-retrieval
error path; it must not change Wi-Fi provisioning or management behavior.

## Locked behavior

- `wifi_management_scan_lock` serializes management scans and is released on
  every path.
- A scan requires a station connection with a DHCP-assigned IPv4 address.
- Keep the existing synchronous scan API and its documented temporary effect on
  station connectivity.
- Preserve scan start, record count, bounded allocation, AP deduplication,
  network cap, sort, result shape, and every existing response/status contract.
- Clear the ESP-IDF AP list after every completed scan attempt, including the
  existing record-retrieval failure path; free allocated records and release
  the scan lock in the same order as the current cleanup paths.
- Preserve event-handler/task synchronization, critical sections, and all
  shared Wi-Fi-state sequencing.
- Preserve credential limits, NVS behavior, erase-before-validation of pending
  credentials, fallback to saved credentials, and zeroization.
- Preserve the intentionally unauthenticated provisioning fallback AP and the
  `management-wifi-routes.c`, `management-device-routes.c`, and `main.c` route
  and header behavior.
- Retain one physical button-release gate after the existing debounce logic;
  remove only the proven unreachable second loop.

## Required validation

- Independent source review against this contract.
- ESP-IDF v6.0.2 build and size check.
- Before release, use the scoped OTA route and confirm post-reboot version,
  update state, HTTPS `443`, read-only NUT `3493`, refused `8080`, and a full
  successful NUT poll. Exercise an authorized browser or API Wi-Fi scan when
  safely available; otherwise record that limitation.
