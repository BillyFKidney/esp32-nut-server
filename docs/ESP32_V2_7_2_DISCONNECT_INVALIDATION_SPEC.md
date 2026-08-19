# v2.7.2 UPS disconnect invalidation and Agent diagnostics

## Goal

When the UPS USB HID device is absent or NUT state is stale, management must
not present cached identity, status, or measurements as current. Provide a
strictly bounded, token-protected simulation so Agent checks precede the final
physical disconnect test.

## Verified variables and invariant

- `usb_hid_device_ready()` reports ESP-IDF HID-device presence.
- `dstate_is_stale()` reports driver freshness; `dstate_dataok()` follows a
  successful driver update.
- The snapshot is stale when HID is unavailable, dstate is stale, or diagnostic
  simulation is active.
- A stale snapshot retains only configured `nut.ups_name`; every external UPS
  field is `unavailable`, with `available=false` and `data_stale=true`.
- Status exposes additive `nut.disconnect_simulated` so a test condition cannot
  be mistaken for a physical failure.

## Credentials and routes

- ADMIN and CSRF protect diagnostic-token issuance, listing, and revocation.
  At most two `diagnostics.nut` tokens are stored under the separate
  `diag-tokens` NVS key; plaintext is shown once only.
- Diagnostic bearer authorization serves Agent status and bounded disconnect
  simulation. Its challenge is diagnostics-specific; absent, malformed,
  revoked, wrong-scope, or oversized credentials are rejected and header
  buffers are zeroized.
- Existing `ota.install` token storage and Agent OTA behavior are unchanged.
  Diagnostic tokens cannot upload; OTA tokens cannot access diagnostics.
- Agent status returns the full management-status JSON without browser-session
  refresh. It intentionally includes the diagnostic, Wi-Fi, update, session,
  and log metadata visible to ADMIN.

## Conditions and actions

- Starting simulation accepts only form-encoded `duration_seconds` from 1 to
  300. Missing/incorrect content type, malformed form data, or out-of-range
  duration changes no state.
- Simulation is RAM-only and task-safe. It never closes USB, alters NVS or
  driver configuration, or controls the UPS.
- While active, the driver marks dstate stale and withholds `dataok`. Expiry or
  an idempotent clear permits recovery only after a later successful driver
  update.
- Status collection observes HID loss directly; the USB callback never calls
  dstate. A real continuing disconnect stays stale after simulation ends.

## Acceptance sequence

1. Build for `esp32s3`, run `git diff --check`, and lint the API probe.
2. Create distinct OTA and diagnostic credentials privately. Verify wrong scope,
   revoked credentials, browser routes, and diagnostic malformed requests fail.
3. With the fingerprint-pinned diagnostic probe, establish baseline status;
   start simulation; verify stale/unavailable UPS fields, the simulation flag,
   and stale NUT state; clear or wait for expiry; verify recovery after a
   successful driver update.
4. Final target acceptance is physical: with no simulation, disconnect UPS USB;
   poll Agent status at one-second intervals and refresh the dashboard. The
   first post-loss response must contain no cached UPS data. Reconnect and
   verify recovery only after a successful driver update.

## Edge cases and rollback

- A reboot clears simulation. A simulation never masks continuing physical
  absence. Clearing it early while HID remains absent leaves data stale.
- Preserve HTTPS `443`, refused `8080`, read-only NUT `3493`, ADMIN/CSRF
  boundaries, OTA validation/rollback, and all UPS-control prohibitions.
- Validate the image and target acceptance before publication. On failure,
  install the target-accepted v2.7.1 image through the normal inactive-slot
  path. Diagnostic-token data is inert to v2.7.1; no NVS migration, erase, or
  physical recovery is required.
