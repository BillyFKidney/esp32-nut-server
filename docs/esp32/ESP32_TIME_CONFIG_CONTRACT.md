# Time-configuration code-review contract

## Scope

This `v2.8.Z` slice resolves the `src/time_config.c` cohesion finding by
extracting only private persistence/schema and timezone policy into a private
module. SNTP state and callbacks, operation locking, manual clock setting,
status projection, and the public `time_config_*` APIs remain in `time_config.c`.

## Locked behavior

- Preserve NVS namespace `management`, key `time-cfg`, version `1`, exact blob
  layout, defaults (`pool.ntp.org`, `America/Los_Angeles`, NTP enabled), and
  load/store migration behavior.
- Preserve NTP-server syntax validation and its 63-character bound.
- Accept only the existing supported IANA timezone list and preserve its
  IANA-to-POSIX mapping, `TZ` assignment, and `tzset()` behavior.
- Keep `active_sntp_server` storage alive for the full ESP-IDF SNTP lifetime.
- Preserve SNTP sequencing: deinitialize the prior instance, set pending,
  initialize with `wait_for_sync=false` and renew-on-new-IP, clear pending on
  failure, and retain the callback behavior.
- Preserve `time_state_lock` and `time_operation_mutex` ownership and all
  currently serialized public operations.
- Preserve manual-date format, 2024–2099 range, timezone round-trip validation,
  `settimeofday`, management status codes/messages, CSRF boundaries, and
  zeroization.
- Register each new `src/*.c` module explicitly in `src/CMakeLists.txt`.

## Required validation

- Independent source review against this contract.
- ESP-IDF v6.0.2 build and size check.
- Before release, validate all 1Password variables redacted, use scoped OTA,
  then confirm post-reboot version/update state, HTTPS `443`, read-only NUT
  `3493`, refused `8080`, and a full successful NUT poll.
