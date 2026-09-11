# ESP32-NUT `/src` code-review coverage

This record closes the persistence gap between branch-local issue trackers and
the fresh full-source review. It covers all 73 tracked C implementation files
under `src` at the `main` baseline `f10f858c2`, plus the focused candidate branches listed
below. Each file received the 68-check Code Complete, AI-slop, and opinionated
practice scan on 2026-09-11.

## Completed focused fixes

| File | Outcome | Evidence |
| --- | --- | --- |
| `src/wifi.c` | Fallback portal lifecycle extracted without behavior change. | `review/wifi-portal-lifecycle` / `v2.8.13` candidate |
| `src/dns-server.c` | Timed-out DNS worker now owns deferred cleanup safely. | `review/dns-server-lifecycle` / `v2.8.14` candidate |
| `src/wifi-credentials.c` | Active/pending NVS flows share fixed private helpers. | `review/wifi-credentials-storage` / `v2.8.15` candidate |
| `src/nut-diagnostics.c` | Diagnostic duration conversion is named. | `review/nut-diagnostics-time-constant` / `v2.8.16` candidate |
| `src/time-config-storage.c` | Standard-library imports are sorted. | `review/time-storage-import-order` / `v2.8.17` candidate |

## Explicit compatibility-contract debt

The following 22 files have real findings, all recorded as `SKIP` in
`review/inherited-compatibility-debt`. They require a dedicated storage,
protocol, hardware, portability, or upstream-compatibility contract and are
not appropriate for an unscoped cleanup release.

- `src/api-token-store.c`, `src/usb.c`
- `src/common/common.c`, `src/common/parseconf.c`, `src/common/snprintf.c`, `src/common/state.c`, `src/common/str.c`, `src/common/strerror.c`
- `src/drivers/apc-hid.c`, `src/drivers/cps-hid.c`, `src/drivers/dstate.c`, `src/drivers/espusb.c`, `src/drivers/hidparser.c`, `src/drivers/libhid.c`, `src/drivers/main.c`, `src/drivers/usb-common.c`, `src/drivers/usbhid-ups.c`
- `src/server/conf.c`, `src/server/netssl.c`, `src/server/sstate.c`, `src/server/upsd.c`, `src/server/user.c`

## Clean scan coverage

No real issue was found in the remaining 46 files:

- `src/api_tokens.c`, `src/main.c`, `src/management-auth-routes.c`, `src/management-authorization.c`, `src/management-certificates.c`, `src/management-credentials.c`, `src/management-device-config.c`, `src/management-device-routes.c`, `src/management-diagnostics-routes.c`, `src/management-http.c`, `src/management-log-routes.c`, `src/management-log.c`, `src/management-ota-routes.c`, `src/management-pages.c`, `src/management-routes.c`, `src/management-session-routes.c`, `src/management-session.c`, `src/management-status-routes.c`, `src/management-status.c`, `src/management-time-routes.c`, `src/management-token-routes.c`, `src/management-wifi-routes.c`, `src/management.c`, `src/ota.c`, `src/time_config.c`, `src/wifi-diagnostics.c`, `src/wifi-portal.c`, `src/wifi-provisioning-web.c`
- `src/common/atexit.c`, `src/common/common-nut_version.c`, `src/common/setenv.c`, `src/common/strnlen.c`, `src/common/strptime.c`, `src/common/strsep.c`, `src/common/timegm_fallback.c`, `src/common/unsetenv.c`, `src/common/upsconf.c`
- `src/drivers/explore-hid.c`, `src/drivers/upsdrvquery.c`
- `src/server/desc.c`, `src/server/netget.c`, `src/server/netinstcmd.c`, `src/server/netlist.c`, `src/server/netmisc.c`, `src/server/netset.c`, `src/server/netuser.c`

## Acceptance state

All focused candidates pass independent source review and ESP-IDF v6.0.2
builds. Publication remains blocked until the Agent Tests and Garage
1Password TLS/token metadata is reconciled and the v2.8.13 captive-portal
path receives a credential-safe runtime acceptance.
