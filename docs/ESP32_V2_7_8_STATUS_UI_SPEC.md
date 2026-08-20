# v2.7.8 — status naming and Device Status presentation

## Goal and release gate

Starting only from published, target-accepted `v2.7.7`, correct the management
status contract and browser presentation without changing UPS behavior,
authorization, ports, or persistence. Rename `nut.ups_name` to `nut.ups`,
show the physical manufacturer/model—not the configured NUT service name—in
the dashboard Model field, and make Device Status raw JSON expanded by default.

This is an intentional API field rename: the old `nut.ups_name` key is removed
rather than retained as an alias. `nut.ups` remains the configured NUT service
identity; it is not physical UPS identity.

## GPT-5.4-mini execution plan

Use `gpt-5.4-mini` at **medium** reasoning. Keep one branch,
`feature/status-naming-and-device-status-ui`, and do not mix it with reset,
device-configuration, token, driver, or OTA protocol changes.

1. **Trace one status contract.** Inspect only
   `management-status.c`, `management-status-routes.c`, `management-pages.c`,
   Agent status reuse, and direct tests/tools. Locate every serialized or
   rendered use of `ups_name`; do not change NUT's configured service name.
2. **Make the contract exact.** Serialize `nut.ups` once in browser and Agent
   status, remove `nut.ups_name`, and update all in-tree consumers/examples.
   Dashboard Model renders available manufacturer and model only, with the
   established unavailable rendering when neither value is current.
3. **Make status visible.** Emit the Device Status `<details>` control with
   `open` so raw JSON is expanded on first render and normal navigation/page
   refresh. Do not persist a browser preference or add background refresh.
4. **Validate.** Run `git diff --check`, the ESP-IDF v6.0.2 `esp32s3` build,
   and authenticated OTA installation (permitted from v2.7.6 onward). Verify
   browser and diagnostic-bearer status JSON, dashboard rendering, stale
   presentation, session non-refresh behavior, ports, and read-only NUT.

## Acceptance and edge cases

| Check | Required result |
| --- | --- |
| Healthy browser and Agent status | Contains `nut.ups`; does not contain `nut.ups_name`; service identity remains correct. |
| Dashboard Model | Shows physical manufacturer/model only; never prefixes the configured service name. |
| Device Status | Raw JSON starts expanded after navigation and browser reload. |
| Stale/unavailable UPS | Model remains unavailable under the v2.7.2/2.7.3 freshness contract; no cached value is presented as current. |
| Security/services | ADMIN session/CSRF, bearer isolation, HTTPS `443`, NUT `3493`, refused `8080`, and UPS read-only policy are unchanged. |

Do not add a compatibility alias, a new API endpoint, NVS data, token scope, or
UPS command. An API-contract regression, stale-data leak, session refresh,
browser rendering failure, or unexpected service exposure stops the release.
Roll back through authenticated dual OTA to accepted `v2.7.7` if validation
fails.

## Documentation and evidence closeout

Update `NAVIGATION.md` if its status-contract wording changes; update API/test
guidance, current status, and development plan with the intentional field
rename. Before authorized publication, create `docs/archive/v2.7.8/evidence.md`
with exact JSON-contract, browser, build, OTA, and rollback evidence. Archive
this specification, update the archive index and roadmap links, and keep all
credentials, addresses, and certificate data out of evidence.
