# v2.7.9 device identity and retained log level validation evidence

## Validation target

- Candidate: `v2.7.9` device identity, retained log level, and
  status-response stack-pressure repair
- Source merge commit: `8ec3b4d49`; final release tag and artifact provenance
  are recorded after publication
- Build target: YD-ESP32-23 / ESP32-S3-WROOM-1-N16R8, ESP-IDF v6.0.2, `esp32s3`
- Current firmware string on target: `v2.7.8-2-g9864b12ab-dirty`

## Implementation boundary

This slice adds a persisted device display name and log-level selector in the
`management` namespace. The display name is trimmed, bounded, and normalized
into a safe derived hostname for the station interface. The configured log
level is applied at boot and after an authenticated ADMIN save. The release
does not change Wi-Fi credentials, token scopes, UPS configuration, the
read-only NUT service, or the factory-reset scope.

## Validation

- `git diff --check`: passed.
- ESP-IDF v6.0.2 `idf.py build` for `esp32s3`: passed.
- The clean pre-release source build after removing temporary reset
  instrumentation passed; the generated application image had 60% partition
  headroom. Existing NUT `fork`/`kill` linker warnings remained unchanged.
- Authenticated OTA install of the current build after factory reset: passed.
- ADMIN save of `device_name=ESP32-NUT Lab` and `log_level=debug` before reset: passed.
- Missing-CSRF POST to `/api/v1/admin/device`: rejected with HTTP 403.
- Save response returned `device_name=ESP32-NUT Lab`, `hostname=esp32-nut-lab`,
  and `log_level=debug`.
- Diagnostics status after save reported the same device name, hostname, and
  log level.
- Factory reset cleared the saved device name and log level; the device
  returned to `device_name=ESP32-NUT`, `hostname=esp32-nut`, and
  `log_level=info` after reconnect and status refresh: passed.
- Live admin-page script load after OTA and factory reset: passed. The browser
  script imported successfully without a syntax error, and the form submit
  handler attached as expected.
- Device settings submit handler intercepted the form submit, prevented the
  default navigation, and POSTed `device_name=ESP32-NUT&log_level=info` to
  `/api/v1/admin/device`: passed.
- Post-reset status reported healthy read-only NUT data after a full
  successful poll.
- Instrumented status exposed the last reset cause and serial capture
  reproduced a `LoadProhibited` panic in `pthread_getspecific()` while
  `management_status_send()` sent the status response; the backtrace identified
  the 7 KB response buffer as HTTPS task-stack pressure.
- Moving the status response buffer to heap storage passed `git diff --check`
  and a clean ESP-IDF v6.0.2 build, then installed through authenticated OTA.
- Post-fix authenticated status-load validation returned repeated HTTP 200
  responses with uptime advancing from 113 through 163 seconds, healthy
  read-only NUT data, and no new panic in the attached serial monitor.
- The renamed 1Password environment variables were present with expected
  lengths without exposing their values: API token length 76, diagnostic token
  length 76, and certificate fingerprint length 64.
- A valid diagnostic bearer returned HTTP 200 with healthy, current NUT data;
  the OTA bearer was rejected by the diagnostic route and the diagnostic
  bearer was rejected by the Agent OTA route with HTTP 401.
- Valid diagnostic simulation at one second and 310 seconds returned HTTP 200
  and was cleared after each run. Durations 0, 311, and nonnumeric input were
  rejected with HTTP 400; JSON content type was rejected with HTTP 415; an
  oversized authorization header was rejected with HTTP 401.
- A valid OTA bearer reached OTA content validation with HTTP 415 for a
  non-firmware content type, without installing an image.
- ADMIN token inventory began and ended at one active API token of four and
  one active diagnostic token of two. Temporary tokens filled the remaining
  slots, overflow creation returned HTTP 409, exact 32-character names were
  accepted, invalid 33-character, illegal-character, and edge-space names
  returned HTTP 400, and all temporary tokens were deleted successfully.
- The target certificate fingerprint matched the authorized 1Password value;
  read-only NUT port 3493 was open. Ten additional authenticated status
  requests returned HTTP 200 with healthy NUT data and uptime advancing from
  70524 to 70534 seconds.

## Boundaries and gaps

- This evidence records a validated pre-release candidate. A clean tagged
  build, OTA installation of the tagged artifact, publication, and remote
  release assets remain separate release steps.
