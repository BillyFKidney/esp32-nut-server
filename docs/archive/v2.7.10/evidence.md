# v2.7.10 release evidence

## Source and artifact

- Release tag: `v2.7.10`, annotated at merged `main` commit
  `6c901bb94f6e861e571bb19e329f63ad3464f132`.
- Target: YD-ESP32-23 / ESP32-S3-WROOM-1-N16R8 on ESP-IDF v6.0.2.
- Source build output: `build/nut-esp32s3.bin`.
- Published-asset names: `nut-esp32s3-v2.7.10.bin` and
  `nut-esp32s3-v2.7.10.bin.sha256`.
- SHA-256: `4181ced51f61e8b135f721c94904c9fd06d88affefe2b2cbf87dd00de6e6f0d9`.
- Sidecar verification against the versioned asset passed before installation.
- The tagged build reports firmware version `v2.7.10`, and the app occupies
  40% of the smallest application partition (60% free).

## Automated and API validation

- `git diff --check` passed before the release commits.
- A clean ESP-IDF reconfigure/build passed from the merged tag source.
- The macOS embedded ADMIN-page validator passed and measured 43,137 UTF-8
  bytes within the bounded 49,152-byte allocation.
- Certificate-pinned diagnostic status was `200`; scoped OTA installation
  succeeded; bearer access to the ADMIN full-log route was rejected with
  `401`.
- The ADMIN full-log route returned the complete current 24-entry ring in
  oldest-to-newest order, with the factual retained capacity and six-entry
  status window. Its read did not refresh the idle-session deadline.
- HTTPS `443` and read-only NUT `3493` responded; retired `8080` was refused.

## Browser and target acceptance

- The original expanded ADMIN page no longer fell back to the buffer-exceeded
  response after sign-in.
- Chrome and iPhone Safari accepted the presentation-only `CPS` to
  `CyberPower Systems` label while the copied raw JSON remained unchanged.
- Device Status displayed the copy controls, then the device-settings form,
  then continuously visible raw JSON. `Copy JSON` and `Copy Logs` worked on
  both browsers; the latter advanced correctly as the volatile ring rolled.
- The Dashboard and Wi-Fi scan/selection flow remained usable. No Wi-Fi
  configuration change was saved during acceptance.
- After the 15-minute idle timeout, a manual Copy Logs attempt returned to
  normal sign-in before content was copied.
- The clipboard-denial fallback is implemented but was not deliberately
  forced; both tested browsers supported clipboard writes.
- The authorized test unit accepted the tagged OTA in `app0`, retained its
  configuration, and completed a post-reboot full NUT poll with health `ok`
  and UPS status `OL`.

## Scope and publication

- NUT client-write reset messages observed during acceptance are outside this
  UI/API release slice; they did not make the target stale or change UPS
  status.
- GitHub-release publication is authorized and follows this evidence commit.
