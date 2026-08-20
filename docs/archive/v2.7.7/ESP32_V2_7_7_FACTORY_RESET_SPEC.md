# v2.7.7 — factory reset clears defined user state

## Goal and release gate

Starting only from published, target-accepted `v2.7.6`, make a BOOT-button
hold of fifteen seconds or more erase every defined user-owned persisted value
and restart into normal Wi-Fi provisioning. Firmware, ESP-IDF boot/OTA
metadata, partitions, and the physical recovery mechanism remain intact.

The current three-second Wi-Fi reset remains a Wi-Fi-only action. The
fifteen-second factory reset must erase both `wifi-config` and the complete
`management` NVS namespace, which contains ADMIN credentials, certificate
material, OTA/diagnostic token verifiers, time configuration, OTA result, and
future device configuration. A RAM-only UPS cache must not survive restart.

## GPT-5.4-mini execution plan

Use `gpt-5.4-mini` at **high** reasoning. This is the minimum suitable model
because this slice is destructive, crosses NVS ownership, and has recovery
semantics. Keep one branch, `feature/factory-reset-clears-state`, and one
reviewable acceptance boundary. Read `AGENTS.md`, current status, this file,
and direct call paths only; never log or expose credentials, tokens, cookies,
or certificate keys.

1. **Inventory and lock scope.** Inspect `src/wifi.c: wifi_recovery_task()`,
   `src/management.c: management_factory_reset()`, and every direct NVS owner.
   Verify the only user-owned namespaces are `wifi-config` and `management`.
   If another user-owned namespace exists, add it explicitly to the reset
   inventory before code changes; do not erase the NVS partition wholesale.
2. **Make reset atomic by namespace.** Reuse bounded namespace erase/commit
   helpers. Do not report success, restart, or clear the session unless every
   required erase/commit has succeeded. A failure leaves the device running,
   logs one bounded error without secrets, and permits a later physical retry.
3. **Preserve gesture and recovery boundaries.** Keep the existing three- and
   fifteen-second thresholds, release-to-confirm behavior, recovery restart,
   HTTPS `443`, read-only NUT `3493`, refused `8080`, and no UPS control.
   Certificate regeneration after reboot is expected; factory reset must not
   erase firmware, OTA slots, or bootloader state.
4. **Build and target validate.** Run `git diff --check` and ESP-IDF v6.0.2
   `esp32s3` build. Authenticated OTA installation is permitted for this
   release; use the separate scoped OTA credential only through 1Password MCP.
   The physical BOOT hold, factory erase, Wi-Fi reprovisioning, and new ADMIN
   setup are the final destructive acceptance and require Device Operator
   authorization.

## Required acceptance

| Scenario | Required result |
| --- | --- |
| Three-second hold | Only saved Wi-Fi/pending Wi-Fi data is erased; ADMIN access and management configuration remain until the device restarts/provisions normally. |
| Fifteen-second hold | `wifi-config` and `management` are erased; prior ADMIN password, both token kinds, time settings, certificate, device configuration, and OTA-result record are inaccessible. |
| Post-reset boot | Device enters provisioning, serves a newly generated HTTPS identity, requires fresh ADMIN setup, and reports no prior UPS data as current. |
| OTA/recovery | Firmware and bootable OTA image remain intact; normal authenticated OTA works again after fresh setup. |
| Failure injection/NVS error | No partial success claim, no unwanted restart, no secret disclosure, and a physical retry remains possible. |

## Edge cases and rollback

- Test absent UPS, active/stale UPS, active browser session, issued/revoked
  token, invalid/changed time, and a reset begun while Wi-Fi is disconnected.
- Never use a browser/API endpoint to trigger factory reset in this slice.
- A failed erase, boot loop, unrecoverable certificate/startup failure, changed
  service port, retained credential, or erased OTA recovery path stops release.
- Roll back to target-accepted `v2.7.6` through authenticated dual OTA before
  a destructive test; after a successful factory reset, recovery means normal
  physical provisioning rather than restoring deleted user state.

## Documentation and evidence closeout

Before publication, update the development plan and current status; create
`docs/archive/v2.7.7/evidence.md` with reset inventory, build/OTA result,
physical gesture evidence, observed/not-tested rows, and rollback outcome.
After authorized tag/release, archive this specification, update
`docs/archive/README.md`, compact current status to the published baseline,
and verify every changed documentation link. Never include secrets, device
addresses, certificate material, or full reset credentials in evidence.
