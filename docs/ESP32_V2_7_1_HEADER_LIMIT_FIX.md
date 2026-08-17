# v2.7.1 management request-header limit repair

## Status

Implementation and target validation completed. This compatible recovery patch
is included in the `v2.7.1` maintenance release and is based directly on the
published `v2.7.0` source tag.

## Observed behavior

On 2026-07-29, a freshly restored `v2.7.0` device successfully entered Wi-Fi
provisioning, joined the configured network, and displayed the initial ADMIN
setup form through the trusted reverse-proxy hostname. Submitting the form from
Chrome returned `431 Header fields are too long` before the password setup
handler ran. Clearing browser data did not change that result.

## Cause

ESP-IDF v6.0.2 defaults `httpd_config_t.max_req_hdr_len` to 1024 bytes. The
combination of modern browser request headers, the device hostname, and trusted
reverse-proxy forwarding headers exceeds that bounded default. A minimal curl
request can reach the setup handler because it sends substantially fewer
headers; that does not reproduce the browser request shape.

## Scoped implementation

`management_server_start()` explicitly sets the HTTPS management server's
request-header limit to 4096 bytes. The setting is runtime-local to HTTPS
management:

- HTTPS management on TCP 443 receives the larger, finite limit.
- The HTTP Wi-Fi captive portal keeps its existing default.
- No authentication, CSRF, cookie, certificate, NVS, UPS, or NUT behavior is
  changed.
- The larger value is a maximum; ESP-IDF allocates according to actual request
  size rather than reserving 4 KiB for every request.

## Validation sequence

1. Build with ESP-IDF v6.0.2 for `esp32s3`.
2. Flash the recovery candidate without erasing NVS or FAT partitions.
3. Through the trusted hostname, submit an intentionally invalid short password
   and confirm that the request reaches the setup handler instead of returning
   `431`.
4. Set the real ADMIN password locally in the browser without recording it in
   source control, terminal output, screenshots, or documentation.
5. Confirm HTTPS 443, read-only NUT 3493, and retired HTTP 8080 boundaries
   remain unchanged.

## Rollback

The patch changes one runtime HTTP-server configuration field. Reflashing the
published `v2.7.0` image restores the prior behavior without erasing persisted
device configuration.
