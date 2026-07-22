# ESP32-NUT management through Synology reverse proxy

This document describes the current trusted-browser deployment for
ESP32-NUT. It is a LAN reverse-proxy procedure, not a replacement for the
future direct-device local-CA milestone.

## Trust model

```text
Browser -- HTTPS/public wildcard --> Synology reverse proxy -- HTTPS --> ESP32-NUT
                                      trusted certificate                 device-specific
                                      terminates here                      self-signed certificate
```

The browser sees the Synology certificate for the public hostname. Synology
connects to the ESP32 over HTTPS and accepts the device's self-signed backend
certificate by local policy. The wildcard private key stays on Synology and
is not provisioned to the ESP32.

This provides encrypted browser-to-device traffic through two HTTPS hops. It
does not provide independent certificate validation of the ESP32 by Synology;
the LAN, Synology, and reverse-proxy configuration are part of the trust
boundary. A future local-CA leaf certificate can harden that backend hop if
the threat model requires it.

## Current hostname assignments

These are the observed local assignments at the time this procedure was
written. The addresses are DHCP observations and must be rediscovered when
they change.

| Device | Browser hostname | Observed backend address |
| --- | --- | --- |
| 3D printer | `esp32nut-3dprinter.28670avenidacondesa.com` | `192.168.40.173` |
| Garage | `esp32nut-garage.28670avenidacondesa.com` | `192.168.40.87` |

Use the hostname—not the backend IP—in Safari, Chrome, the Agent OTA helper,
and other browser-facing management clients.

## AdGuard DNS rewrite

For the reverse proxy to serve the wildcard certificate, each browser
hostname must resolve on the LAN to the Synology reverse-proxy listener. The
Synology rule then selects the correct ESP32 backend by hostname.

If AdGuard resolves the hostname directly to the ESP32, the request bypasses
Synology and the browser will see the ESP32's self-signed certificate instead.

After changing a rewrite, verify the answer from the client and allow for DNS
cache expiration before diagnosing the proxy:

```text
esp32nut-3dprinter.28670avenidacondesa.com -> Synology reverse-proxy address
esp32nut-garage.28670avenidacondesa.com     -> Synology reverse-proxy address
```

## Synology reverse-proxy rule

Create one HTTPS rule per board:

- Source hostname: the board's browser hostname
- Source port: `443`
- Certificate: the trusted `*.28670AvenidaCondesa.com` certificate
- Backend protocol: `HTTPS`
- Backend hostname/address: the current ESP32 address
- Backend port: `443`
- Backend certificate behavior: accept the device-specific self-signed
  certificate according to the approved local policy

Do not expose the reverse proxy or ESP32 management interface outside the
trusted LAN. Do not restore the retired unauthenticated ESP32 TCP `8080`
listener.

## Forwarded headers

The current working rule forwards these headers:

| Header | Value |
| --- | --- |
| `Upgrade` | `$http_upgrade` |
| `Connection` | `$connection_upgrade` |
| `Host` | `$host` |
| `X-Real-IP` | `$remote_addr` |
| `X-Forwarded-For` | `$proxy_add_x_forwarded_for` |
| `X-Forwarded-Proto` | `$scheme` |
| `X-Forwarded-Port` | `$server_port` |
| `X-Forwarded-Host` | `$host` |

The ESP32-NUT management server currently uses the session cookie,
`Authorization`, and `X-ESP32-NUT-CSRF` headers. It does not use the forwarded
headers for authentication or authorization. Do not treat `X-Real-IP` or
`X-Forwarded-For` as an authentication signal.

## Verification checklist

For each hostname:

- [ ] The browser URL uses the hostname, not the ESP32 IP address.
- [ ] The browser's leaf certificate is the configured public wildcard
      certificate, and its SAN covers the requested hostname.
- [ ] The ADMIN page loads and login succeeds.
- [ ] Dashboard/status requests succeed through the proxy.
- [ ] A CSRF-protected state change succeeds only with the existing ADMIN
      session and CSRF header.
- [ ] Local firmware check/install behaves normally through the proxy.
- [ ] The Synology backend target is HTTPS on ESP32 port `443`.
- [ ] Direct network checks still show ESP32 TCP `443` and read-only NUT TCP
      `3493` available, with retired TCP `8080` refused.
- [ ] The UPS remains read-only and reports the expected `ups.status`.

The management page may still say that HTTPS uses the device's self-signed
certificate. That text describes the ESP32 backend implementation and is not a
live report of the certificate presented by Synology's browser-facing hop.

## Recovery and maintenance

When a board receives a new DHCP address:

1. Rediscover the board on the LAN.
2. Update the matching Synology backend target.
3. Update the AdGuard rewrite only if the browser hostname no longer reaches
   Synology.
4. Re-run the verification checklist.

If a fifteen-second factory reset regenerates the ESP32 certificate, the
Synology backend's accepted certificate state may need to be refreshed. Do not
erase certificate NVS blobs as a migration shortcut; the current firmware
regenerates another self-signed certificate when the material is missing.

## Future direct-device trust

If the project later requires Synology to authenticate each ESP32 directly,
use a separate per-device certificate issued by the reviewed local CA. Do not
install the shared wildcard private key on an ESP32. That future work belongs to
the `feature/local-ca-trust` / `v3.0.0` slice and requires its own enrollment,
renewal, reset, and revocation review.
