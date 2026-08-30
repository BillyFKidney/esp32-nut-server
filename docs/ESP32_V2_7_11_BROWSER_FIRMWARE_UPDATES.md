# v2.7.11 browser-based firmware updates

## Scope

`v2.7.11` is a maintenance bugfix for the authenticated, browser-based local
firmware check and install flow. It is intentionally separate from the deferred
`v2.8.0` optimization work.

## Observed finding

On the authorized `3Dprinter` v2.7.10 test device, selecting the published
v2.7.10 application image and pressing **Check firmware** showed “Unable to
reach the firmware check service.” The same symptom was reported on v2.7.9.
The browser was authenticated, the update page rendered, and the image selector
accepted the local file. The v2.7.11 candidate then reproduced the request
through the browser hostname and reported `HTTP 413`.

## Root-cause assessment

The firmware-check route remains registered and protected by the ADMIN session,
CSRF header, and `application/octet-stream` requirement. The browser page,
however, could start a document-wide session-activity request and an in-flight
status request at the same time as the large OTA check upload. Its broad catch
block then presented network failure, non-JSON response, and HTTP failure as the
same “unreachable” message. The post-install reconnect helper was separately
an unbounded recursive retry.

The reachable route then identified the operational blocker: the Synology NGINX
reverse proxy returned its own `413 Request Entity Too Large` HTML response
before contacting the ESP32. The stock 1 MiB NGINX request-body limit is below
the roughly 1.35 MiB current application image. The ESP32 still independently
limits its inactive OTA partition to `0x330000` (3,342,336 bytes).

This assessment does not claim a firmware-check endpoint removal or a change to
the read-only NUT/UPS path.

## v2.7.11 behavior

- Block session-activity and status-refresh requests while a browser OTA check
  or install is active, and wait for any already-active request before upload.
- Keep one browser OTA operation in flight and apply a bounded 120-second
  browser request timeout.
- Show non-JSON, HTTP, and browser transport failures distinctly.
- Replace post-install recursive reconnect retries with a bounded two-minute
  retry loop that returns control to the operator when the device does not
  return.
- Require a host-specific reverse-proxy upload limit above the current image
  but below a broad global allowance; the documented `4m` limit remains bounded
  by the ESP32's smaller partition validation.
- Preserve the local raw-image-only update design, ADMIN session/CSRF checks,
  inactive-slot validation, and install-only boot selection/reboot behavior.

## Acceptance

1. A valid local application image checks through the ADMIN browser page and
   reports its embedded version without a reboot or boot-slot selection.
2. The same operation with automatic status refresh enabled creates no competing
   status/session request and leaves the buttons usable afterward.
3. Error responses retain the actual HTTP/response failure context without
   exposing credentials or tokens.
4. A valid install still selects the inactive slot, restarts once, and returns
   with a successful NUT poll and the HTTPS/NUT service boundaries intact.
5. Invalid CSRF, wrong content type, malformed images, and concurrent requests
   retain their existing rejection behavior.
6. The approved browser hostname forwards a valid image through the proxy after
   its host-specific `4m` body limit and 130-second proxy timeouts are applied.
