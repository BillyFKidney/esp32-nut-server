# ESP32-NUT security contract

This is the active security and authorization contract. Locked product
decisions are in
[ESP32_DEVELOPMENT_MILESTONE_QA_OPERATIONAL_MANAGEMENT.md](ESP32_DEVELOPMENT_MILESTONE_QA_OPERATIONAL_MANAGEMENT.md).
The detailed v2.7.1 snapshot, including generic and superseded guidance, is
archived in [ESP32_SECURITY_V2_7_1.md](archive/ESP32_SECURITY_V2_7_1.md).

## Non-negotiable boundaries

- Management is LAN-only HTTPS on TCP `443`. The retired unauthenticated
  service on TCP `8080` remains refused.
- The device uses a self-signed certificate until reviewed local-CA work. Never
  expose the management interface outside the trusted LAN.
- NUT remains read-only on TCP `3493`. `SET`, `INSTCMD`, `FSD`, NUT users, and
  UPS controls are out of scope until a separately reviewed safety model
  authorizes them.
- Do not record passwords, Wi-Fi credentials, session cookies, CSRF values,
  complete API tokens, private keys, certificate material, or Authorization
  headers in source, documentation, logs, screenshots, terminal output, or
  chat.

## Provisioning, storage, and physical access

Wi-Fi provisioning is intentionally open only when setup or recovery is
required. Provision from a trusted physical location, complete it promptly,
and do not treat the setup-network name as an ADMIN secret. Wi-Fi credentials
are submitted as pending NVS data and become active only after a station-only
connection check succeeds; failed staging preserves the prior active network.

Initial setup requires the owner to choose the ADMIN password twice. The device
stores a salted password verifier, not the password. Physical recovery and
factory reset follow their separately locked scopes; reset, decommissioning,
and hostile-environment deployment require explicit physical-security review.
Flash encryption and secure boot are deferred platform-security work, not
current defaults.

The FAT-backed NUT configuration does not provide a Unix permission boundary
on this target. No NUT users or password material are configured in the
read-only milestone; do not rely on ownership or `chmod`-style controls for
device security.

## Browser administration

ADMIN browser sessions use Secure, HttpOnly, SameSite cookies and a
server-authoritative fifteen-minute idle deadline. State-changing browser
requests require CSRF protection. Login attempts are throttled. Normal ADMIN
activity may extend the deadline; background status refresh must not. Expiry,
or an authenticated `401`/`403`, returns the browser to sign-in.

Wi-Fi, password, token, time, and local-OTA changes remain ADMIN-session and
CSRF protected. Wi-Fi scanning and credential staging never disclose the
stored password; the browser-only Show-password control is not persisted,
returned through an API, or logged.

## API tokens, diagnostics, and Agent OTA

Tokens are device-generated, scoped bearer credentials. Show a complete token
only once at creation; retain only non-secret metadata and a salted verifier.
Send it only as an HTTPS `Authorization: Bearer` header. Do not accept or send
tokens in query parameters, cookies, source files, shell history, or logs.
Compare verifiers in constant time and zeroize Authorization-header storage
after use.

OTA tokens use the `ota.install` scope and authorize only the Agent OTA
installation route. Diagnostic tokens use the `diagnostics.nut` scope and
authorize only Agent diagnostic status and bounded disconnect simulation. The
two token sets have separate NVS stores: OTA storage remains compatible with
v2.7.1 rollback, and a diagnostic credential can never authorize OTA.

A diagnostic credential intentionally reads the same management-status
diagnostics visible to ADMIN, including diagnostic, Wi-Fi, update, session, and
log metadata. Treat it as sensitive and revoke it if disclosure is suspected.
Diagnostic simulation is RAM-only, bounded, cannot operate USB or the UPS, and
clears on expiry or reboot. Browser pages, browser status, passwords, time,
token management, logout, Wi-Fi, and local OTA remain ADMIN-session and CSRF
actions; bearer requests do not refresh browser activity.

Agent helpers retrieve their scope-specific token and certificate-trust material
only from the 1Password MCP developer environment, then validate the device
certificate with a trusted CA file or verified fingerprint before sending a
request. Insecure self-signed diagnostics are temporary only and must not be
used with a valuable credential. Revoke and replace a possibly disclosed token;
do not attempt to recover it.

## Update, network, and resource safety

Local OTA accepts only a raw ESP-IDF application image, validates it before
selecting the inactive slot, and relies on ESP-IDF rollback if the new image
does not become valid. Production remote OTA, signed metadata, and local-CA
trust are later reviewed milestones; do not restore an unauthenticated update
listener as a substitute.

HTTPS protects management credentials and tokens, not the read-only NUT
service. Use appropriate LAN segmentation and firewall policy for the
deployment. The ESP32 has bounded sockets, heap, flash, and task capacity;
security and reliability testing must include malformed input, authorization
boundaries, concurrent clients, and resource-exhaustion behavior.

## Review, incident response, and reporting

Review security-sensitive changes for authentication, authorization, input
bounds, secret handling, service exposure, recovery, and rollback. Validate
them on the ESP32-S3 in proportion to risk without weakening the service-port
or read-only UPS boundaries.

If compromise is suspected, disconnect the device from the network, preserve
non-secret observations, rotate affected credentials, inspect configuration
and network exposure, then recover through the approved physical and firmware
path. Do not disclose a suspected vulnerability in a public issue; use the
project security contact in root `SECURITY.md`.

## References

- [ESP-IDF security features](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/security/index.html)
- [NUT security documentation](https://networkupstools.org/docs/user-manual.chunked/ar01s06.html)
