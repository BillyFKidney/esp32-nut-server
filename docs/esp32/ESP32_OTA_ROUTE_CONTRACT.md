# OTA route code-review contract

## Scope

This `v2.8.Z` review slice addresses only the routine-boundary finding in
`ota_process_from_request()` and names the existing reboot task configuration.
The implementation may extract private helpers in `src/ota.c`; it must not
change externally observable OTA behavior.

## Locked behavior

- Browser installation remains ADMIN-session plus CSRF protected; Agent
  installation remains limited to the `ota.install` bearer scope.
- Both browser routes retain `application/octet-stream` validation before the
  OTA core is entered.
- `ota/check` validates the image but must neither select a boot partition nor
  persist an install result nor schedule a restart.
- `ota/install` retains the current busy, missing-partition, invalid-size,
  receive-timeout, truncated-receive, write, validation, boot-selection, and
  success HTTP status/message contracts.
- The NVS namespace/key and result transitions remain `management` /
  `ota-result`: record `failed` or `rejected` at the same current failure
  boundaries, record `pending` only after image validation and boot selection,
  and record `installed` only after post-boot validation.
- Retain the 4 KiB receive buffer, four receive-timeout retries, response
  headers, inactive-partition selection, `esp_ota_abort()` behavior, and reboot
  only after a successful install response.
- Retain the existing reboot task stack size and priority; naming those values
  is not authorization to tune them.

## Required validation

- Independent source review of the extraction against this contract.
- ESP-IDF v6.0.2 build and size check.
- Before release, use the scoped OTA route and confirm post-reboot version,
  update state, HTTPS `443`, read-only NUT `3493`, refused `8080`, and a full
  successful NUT poll.
