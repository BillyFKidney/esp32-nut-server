# ESP32-NUT Navigation Cheat Sheet

For branch, release, target, and next-task context, begin with
`docs/ESP32_CURRENT_STATUS.md`; confirm its Git facts before changing code.

## If you're looking for... → Start here

| If you're looking for... | Start here |
| --- | --- |
| Application entry / boot sequence | `src/main.c:app_main()` |
| NUT and HID task launch | `src/main.c:nut_main()`; `src/main.c:drv_main()` |
| Wi-Fi lifecycle / event handling | `src/wifi.c:wifi_provisioning_init()`; `src/wifi.c:wifi_event_handler()` |
| Wi-Fi provisioning / captive portal | `src/wifi.c:wifi_portal_start()`; `src/wifi-provisioning-web.c:wifi_provisioning_web_start()` |
| Captive-portal HTML and routes | `src/wifi-portal.c:wifi_portal_html`; `src/wifi-provisioning-web.c:portal_configure_handler()` |
| Captive-portal DNS | `src/dns-server.c:dns_server_start()` |
| Wi-Fi scan and staged reconnect | `src/wifi.c:wifi_management_scan()`; `src/wifi.c:wifi_management_stage_credentials()` |
| Wi-Fi credentials in NVS | `src/wifi-credentials.c:wifi_credentials_load()`; `src/wifi-credentials.c:wifi_credentials_save()` |
| HTTPS admin-server startup | `src/management.c:management_server_start()` |
| HTTPS admin route registration | `src/management-routes.c:management_routes_register()` |
| Login, setup, and ADMIN password routes | `src/management-auth-routes.c:management_auth_login_handler()`; `src/management-auth-routes.c:management_auth_password_change_handler()` |
| Status JSON and dashboard data | `src/management-status-routes.c:management_status_handler()`; `src/management-status.c:management_status_collect_nut_snapshot()` |
| Device identity and retained log level | `src/management-device-routes.c:management_device_config_handler()`; `src/management-device-config.c:management_device_config_snapshot()`; `src/wifi.c:wifi_provisioning_set_station_hostname()` |
| Agent diagnostic status and disconnect simulation | `src/management-status-routes.c:management_agent_status_handler()`; `src/management-diagnostics-routes.c:management_diagnostic_disconnect_start_handler()` |
| Browser session, idle expiry, and CSRF | `src/management-session.c:management_session_is_authorized()`; `src/management-session.c:management_session_csrf_is_valid()` |
| Shared session / bearer authorization boundary | `src/management-authorization.c:management_require_session()`; `src/management-authorization.c:management_bearer_is_authorized()` |
| Logout and session-activity routes | `src/management-session-routes.c:management_session_logout_handler()`; `src/management-session-routes.c:management_session_activity_handler()` |
| Local and Agent OTA routes | `src/management-ota-routes.c:management_ota_install_handler()`; `src/management-ota-routes.c:management_agent_ota_install_handler()` |
| OTA image validation, write, and rollback validity | `src/ota.c:ota_check_from_request()`; `src/ota.c:ota_install_from_request()`; `src/ota.c:ota_mark_running_image_valid()` |
| API-token persistence and authorization | `src/api_tokens.c:api_tokens_create()`; `src/api_tokens.c:api_tokens_authorize()` |
| Separate diagnostic-token persistence | `src/api_tokens.c:diagnostic_tokens_create()`; `src/api_tokens.c:diagnostic_tokens_authorize()` |
| API-token management routes | `src/management-token-routes.c:management_token_create_handler()`; `src/management-token-routes.c:management_token_delete_handler()` |
| Time configuration and SNTP | `src/time_config.c:time_config_start()`; `src/time_config.c:time_config_update()`; `src/time_config.c:time_config_request_sync()` |
| Time API route | `src/management-time-routes.c:management_time_config_handler()` |
| USB HID UPS polling | `src/drivers/usbhid-ups.c:upsdrv_updateinfo()`; `src/drivers/main.c:drivers_main()` |
| UPS disconnect invalidation / simulation state | `src/nut-diagnostics.c:nut_diagnostics_disconnect_simulation_active()`; `src/management-status.c:management_status_collect_nut_snapshot()` |
| CyberPower HID subdriver selection and mapping | `src/drivers/cps-hid.c:cps_claim()`; `src/drivers/cps-hid.c:cps_hid2nut[]` |
| NUT driver stale lifecycle, timeout, and published values | `src/drivers/dstate.c:dstate_setinfo()`; `src/drivers/dstate.c:dstate_datastale()`; `src/drivers/dstate.c:dstate_stale_timeout_check()` |
| NUT network-server startup and main loop | `src/server/upsd.c:main()`; `src/server/upsd.c:mainloop()` |
| NUT protocol command dispatch | `src/server/netcmds.h:netcmds[]` |
| NUT `GET`, `LIST`, and `SET` handlers | `src/server/netget.c:net_get()`; `src/server/netlist.c:net_list()`; `src/server/netset.c:net_set()` |

## Key architectural boundaries

- `src/main.c` boots ESP-IDF services, starts Wi-Fi, mounts FAT, then launches
  the read-only HID driver and NUT server as separate tasks.
- `src/wifi.c` owns station/AP lifecycle, recovery gestures, credential
  promotion, and the temporary captive portal; it starts management after an IP.
- `src/management.c` owns root-page policy, HTTPS lifecycle, and factory-reset
  orchestration. Focused `management-*.c` modules own routes and state details.
- `src/drivers/usbhid-ups.c` polls USB HID and publishes values through dstate;
  `src/server/upsd.c` serves that dstate through the NUT network protocol.
- Keep management HTTPS on `443`, NUT read-only on `3493`, and retired `8080`
  refused; preserve ADMIN/CSRF and bearer-token boundaries.

## Build and flash reference

```bash
. /Users/billyfkidney/.espressif/v6.0.2/esp-idf/export.sh
idf.py build
# Requires explicit device authority and a rediscovered serial port:
idf.py -p <current-serial-port> flash
idf.py -p <current-serial-port> monitor
```

Use `docs/ESP32_PREFLIGHT.md` before hardware, flash, OTA, reset, or serial
work. This map is an entry-point guide, not permission to alter a device.
