# ESP32-NUT v2.8.0 implementation and acceptance plan

This is the task-specific plan for `feature/optimization`. v2.8.0 is a
focused optimization and appliance-UI slice; it is not an API compatibility
reset or a general redesign of the NUT architecture.

## Locked scope and compatibility boundary

- Preserve every v1 API route, request/response shape, session behavior,
  bearer scope, ADMIN authorization rule, and CSRF payload contract. Existing
  callers must continue to work without migration.
- Preserve LAN-only HTTPS `443`, read-only NUT `3493`, refusal of retired
  unauthenticated `8080`, and the existing NUT daemon/driver architecture.
- Redesign the device-hosted UI as a polished, responsive appliance console
  using self-contained vanilla HTML/CSS/JavaScript. Do not add a framework,
  CDN, remote font, external icon pack, or runtime dependency.
- Reuse the repository's canonical NUT logo as the device-served favicon; do
  not add a remote icon request or duplicate artwork in the ADMIN page.
- Dashboard is an operational summary and does not display logs.
- Device Status retains raw JSON and its six-entry `logs` array exactly as the
  v1 response provides it; display the JSON pretty-printed for diagnosis.
- Add a Logs page that consumes the existing ADMIN-only
  `GET /api/v1/admin/logs` route, displays the newest bounded 24 entries in
  the route's documented order, and provides Copy and Download actions. The
  page must retain authorization, CSRF/session semantics, and clipboard
  fallback behavior; it must not create a second log-retention mechanism.
- Improve implementation-level memory/heap usage and user-perceived
  responsiveness, including measured use of both CPU cores where safe. Do not
  make API-wide payload or allocation changes under this release.

v3.0 owns the intentional API v2 and token redesign, v1 compatibility changes,
and API-wide memory/performance changes. Any proposal that alters a v1
contract or shared API resource budget is out of scope and must be recorded as
a v3.0 handoff instead.

## Implementation workstreams

1. Inventory current routes, page callers, session refresh rules, CSRF fields,
   log ordering, and allocations before editing. Mark findings observed,
   inferred, or not tested.
2. Establish target budgets for app image growth, internal heap, PSRAM,
   minimum free heap, HTTP-task stack, page/JSON allocation, and concurrent
   browser requests. Keep buffers bounded and avoid duplicate whole-response
   copies.
3. Refactor UI presentation into shared vanilla components/styles while
   preserving route contracts and authorization boundaries. Keep management
   and Wi-Fi orchestration boundaries intact.
4. Implement Dashboard, Device Status, and Logs navigation and responsive
   states. Pretty-print raw Device Status JSON; omit logs from Dashboard.
   Render Logs from the existing endpoint only, with bounded DOM content,
   Copy, Download, empty/error, and clipboard-denied fallback states.
5. Evaluate safe work placement across the two cores and task stacks. The
   current USB event/discovery/HID and NUT driver/server tasks deliberately
   share core 0, while unpinned ESP-IDF management/network work may use the
   dual-core scheduler. Do not move driver-owned work merely to claim both-core
   use: require profiling plus USB, NUT, Wi-Fi, watchdog, and rollback evidence
   before changing affinity. v2.8 responsiveness comes from lazy browser work
   and bounded streaming unless that evidence supports a safe affinity change.
6. Add/retain build-time checks for embedded JavaScript syntax, generated-page
   size, and resource budgets. Register any new source module explicitly in
   `src/CMakeLists.txt`.

## Acceptance gates and evidence

Evidence must label each result **observed**, **inferred**, or **not tested**.

### Build and static checks

- Clean ESP-IDF v6.0.2 reconfigure/build for the ESP32-S3 target passes.
- Embedded JavaScript syntax, generated-page size, and bounded-allocation
  checks pass; app-partition headroom and heap/stack budgets are recorded.
- No credentials, cookies, tokens, private keys, or Authorization headers are
  present in source, logs, or evidence.

### Browser and API checks

- On MacBook/Chrome and iPhone/Safari when available, verify navigation,
  responsive layout, session expiry, ADMIN-only access, CSRF failures, and
  bearer-scope behavior.
- Confirm Dashboard has no logs; Device Status shows pretty raw JSON with six
  `logs`; Logs shows the existing route's last 24 entries, Copy and Download,
  and safe empty/error/clipboard-denied states.
- Independently compare v1 payloads and request fields before/after; all
  compatibility checks must pass.

### Device, service, and performance checks

- With explicit target/OTA authorization, build and install only the pinned
  candidate, verify version/app slot, reboot, and complete a successful full
  NUT poll before acceptance. Record UPS state and recovery evidence.
- Verify HTTPS `443`, read-only NUT `3493`, refused `8080`, and no unintended
  network listener. Exercise relevant ADMIN/session/CSRF boundaries on the
  device, not only in source review.
- Measure representative page/API latency, concurrent navigation, free heap,
  minimum heap, PSRAM, and task-stack margins. Compare against the recorded
  baseline and resource budgets; distinguish observation from inference.

### Rollback and release gates

- Preserve a known-good v2.7.10 artifact and document downgrade/rollback,
  failure symptoms, and recovery steps before installation.
- A failed browser, security, NUT, resource, or reboot/full-poll gate blocks
  acceptance. Do not weaken a boundary to make a test pass.
- Merge, tag, publish, flash, or OTA only after the Project Maintainer gives
  the exact required authorization. Release closeout must include evidence,
  versioned firmware, SHA-256 sidecar, rollback/status update, and the exact
  next action; otherwise state that publication was not authorized/performed.

## Explicit non-goals and handoff

No API v2, token migration, v1 field removal, API-wide response reshaping,
automatic OTA, UPS controls, service retirement, or security-posture change is
part of v2.8.0. Capture measured API/resource redesign opportunities for the
v3.0 planner and leave the current v1 contracts as the compatibility baseline.
