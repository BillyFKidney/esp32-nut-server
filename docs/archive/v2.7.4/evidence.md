# v2.7.4 release evidence

## Scope

v2.7.4 broadens the generated read-only `usbhid-ups` configuration so the
evidenced APC Back-UPS RS 1500G and the established CyberPower CST150UC2 can
use the normal NUT HID subdriver selection path. The configured NUT service
name remains unchanged. No UPS-control, NVS, factory-reset, port, or
authorization behavior changed.

## Validation

- ESP-IDF v6.0.2 `esp32s3` build passed.
- `git diff --check` passed.
- OTA installation of the clean v2.7.4 image succeeded.
- APC enumeration, full polling, more than 17 minutes of continuous status
  observation, and three physical disconnect/reconnect cycles passed.
- CyberPower boot, healthy polling, physical disconnect stale invalidation, and
  recovery after reconnect/full poll passed.
- Dashboard presentation matched authenticated status after recovery.
- HTTPS `443` was reachable, read-only NUT `3493` answered `VER`, `LIST UPS`,
  and `GET VAR`, and retired port `8080` was refused.
- Diagnostics bearer status access succeeded; the OTA bearer was rejected
  from the diagnostics route.

## Observed limitation

One earlier candidate observation rebooted near thirteen minutes; a later
continuous observation exceeded seventeen minutes without reproduction. The
maintainer explicitly accepted this unreproduced event for publication. A
newly attached different UPS still requires reboot for this release; immediate
replacement reprobe remains the v2.7.6 scope.

## Provenance

- Merge commit: `8a52c8991`
- Release tag: `v2.7.4`
- Clean application image SHA-256: `dfc3233134d7008bc2ac0cff05e7f2c3af3791984768f59afdfccc6d587d859c`
- GitHub release: https://github.com/BillyFKidney/esp32-nut-server/releases/tag/v2.7.4
- Final target image identity: `v2.7.4`, with OTA result `installed`

No credentials, tokens, cookies, Authorization headers, private keys, serial
numbers, IP addresses, or verification endpoints are recorded here.
