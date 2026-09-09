# v2.7.11 browser-based firmware-update incident

## Scope

The `v2.7.11` branch was opened to investigate an apparent authenticated,
browser-based local firmware-check failure. The investigation is closed: this
was not an ESP32-NUT firmware defect. It was a remote NGINX configuration
error, and no v2.7.11 firmware will be released.

## Observed finding

On the authorized `3Dprinter` v2.7.10 test device, selecting the published
v2.7.10 application image and pressing **Check firmware** showed “Unable to
reach the firmware check service.” The same symptom was reported on v2.7.9.
The browser was authenticated, the update page rendered, and the image selector
accepted the local file. A diagnostic candidate then reproduced the request
through the browser hostname and reported `HTTP 413`.

## Root-cause assessment

The firmware-check route remains registered and protected by the ADMIN session,
CSRF header, and `application/octet-stream` requirement. The diagnostic
candidate distinguished HTTP and non-JSON responses from a browser transport
failure. That reporting exposed the operational blocker; it did not correct the
upload path.

The NGINX site configurations supplied for
`esp32nut-3dprinter.28670avenidacondesa.com` and
`esp32nut-garage.28670avenidacondesa.com` contain no
`client_max_body_size` directive. NGINX therefore applied its stock 1 MiB
request-body limit and returned its own `413 Request Entity Too Large` HTML
response before contacting either ESP32. The current application image is about
1.35 MiB. The ESP32 independently limits the inactive OTA partition to
`0x330000` (3,342,336 bytes).

This assessment does not claim a firmware-check endpoint removal or a change to
the read-only NUT/UPS path.

## Disposition

The candidate was useful diagnostic evidence only. Its source changes are
reverted before this branch merges, leaving a documentation-only record. The
existing local raw-image update design, ADMIN session/CSRF checks,
inactive-slot validation, and install-only boot selection/reboot behavior are
unchanged.

## Proxy remediation and acceptance

1. Each ESP32 management site applies the documented host-specific `4m` body
   limit and 130-second proxy timeouts.
2. A valid local application image checks through the ADMIN browser page and
   reports its embedded version without a reboot or boot-slot selection.
3. A valid install selects the inactive slot, restarts once, and returns with a
   successful NUT poll and the HTTPS/NUT service boundaries intact.
4. Invalid CSRF, wrong content type, malformed images, and concurrent requests
   retain their existing rejection behavior.
