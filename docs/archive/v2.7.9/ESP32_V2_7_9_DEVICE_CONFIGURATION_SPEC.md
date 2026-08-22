# v2.7.9 — device identity and retained log level

## Goal and release gate

Starting only from published, target-accepted `v2.7.8`, provide ADMIN/CSRF
configuration for a memorable device name and retained application log level.
The default remains `ESP32-NUT`. The chosen device name appears in the
management-console title and status response; a validated hostname derived
from it replaces the fixed `expressif` hostname on the station interface.

Persist both settings in the reset-covered `management` namespace, so the
v2.7.7 factory reset clears them without a later reset-scope change. This
release does not change Wi-Fi credentials, API-token scopes, UPS configuration,
NUT service name, or physical factory-reset behavior.

## GPT-5.4-mini execution plan

Use `gpt-5.4-mini` at **medium** reasoning. Keep one branch,
`feature/device-identity-and-log-level`, and one focused configuration module
plus narrow route/page changes. The existing unused `device-name` key must be
either adopted by the validated configuration owner or removed as part of that
same coherent change—never left as duplicate persistence.

1. **Lock the storage and validation contract.** Create/load/save one bounded
   configuration record in `management`; use a versioned layout and atomic
   replacement. Device display names are trimmed, nonempty, bounded, and free
   of control characters. Derive a DNS hostname of at most 63 characters using
   lowercase letters, digits, and interior hyphens; normalize/reject invalid
   input and fall back safely to `esp32-nut` only when required.
2. **Apply identity safely.** Load settings before status/pages are served.
   Use the Wi-Fi-owned station-netif path to set the derived hostname; apply it
   before the next DHCP association and state clearly in the UI if an existing
   lease needs reconnect/reboot to advertise it. Never put the display name in
   a TLS certificate, token, NUT service name, or fixed-size unsafe buffer.
3. **Define log-level scope honestly.** Provide a dropdown limited to
   Error/Warning/Info/Debug/Verbose, persist it, and apply the selected ESP-IDF
   application log threshold at boot and after an ADMIN/CSRF save. Document
   whether inherited NUT-driver verbosity is unaffected; do not promise to
   suppress output that is outside ESP-IDF log-level control or mutate driver
   parsing/control state at runtime.
4. **Build and test.** Run `git diff --check` and ESP-IDF v6.0.2 `esp32s3`
   build. Authenticated OTA installation is permitted. Test valid/invalid
   names, hostname derivation, reboot persistence, default migration, console
   title/status response, log-level persistence, ADMIN/CSRF rejection, and
   factory-reset clearing of both settings.

## Acceptance and stop conditions

| Check | Required result |
| --- | --- |
| New device name | ADMIN save returns a bounded result; title and status use the saved display name after refresh/reboot. |
| Default/no record | Existing devices report and display `ESP32-NUT` without setup or migration failure. |
| Hostname | Derived value is safe, stable, and applied through the Wi-Fi lifecycle without exposing Wi-Fi credentials or breaking station recovery. |
| Log level | Dropdown selection takes effect for the documented log domain and survives reboot; invalid values leave the previous saved configuration unchanged. |
| Factory reset | The fifteen-second reset removes both values; the next boot returns to default name/default level and provisioning. |
| Security/services | ADMIN/CSRF protects changes; bearer scopes do not gain configuration access; HTTPS `443`, NUT `3493`, refused `8080`, OTA isolation, and no UPS control remain unchanged. |

Stop for NVS corruption/loss of recoverability, unsafe hostname behavior,
credential leakage, a session/CSRF bypass, log-induced resource regression,
or a factory-reset omission. Roll back through authenticated dual OTA to
accepted `v2.7.8`; no NVS migration or factory reset is needed for rollback.

## Documentation and evidence closeout

Update navigation for the configuration/hostname/log entry points, security
only if the authorization contract changes, preflight if the hostname recovery
procedure changes, current status, and development plan. Before authorized
publication, create `docs/archive/v2.7.9/evidence.md` with configuration,
reboot, hostname, log-level, reset, build, OTA, and rollback results. Archive
this specification, update the archive index and roadmap links, and omit all
credentials, device addresses, tokens, and certificate material.
