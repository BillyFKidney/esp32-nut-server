# 2026-10-09 PinBoard APC/CyberPower replacement evidence

## Scope

Current-firmware read-only NUT and physical USB replacement check on PinBoard.
No source, firmware, configuration, or NVS change was made by the agent.

## Target and starting state

- Board: YD-ESP32-23 / ESP32-S3-WROOM-1-N16R8.
- Reported firmware: `v2.9.5-1-gd51fe17bc-dirty`, running in `app0`.
- The Maintainer reported reimaging the board and performing a full reset before
  this test. The provided pre-swap status snapshot already showed the APC
  Back-UPS RS 1500G fresh with `ups.status=OL`; the earlier stale condition was
  not present in that snapshot.
- The Maintainer physically changed only the UPS-side USB connection. Both UPS
  units remained on AC with their loads powered; the board remained up.

## Observed results

### APC Back-UPS RS 1500G to CyberPower CST150UC2

- The first post-swap diagnostic sample showed NUT stale and UPS fields
  unavailable. Logs showed HID connect/start and selection of the CyberPower
  HID subdriver `0.84`.
- The next successful poll identified `CST150UC2`, cleared stale state, and
  returned `ups.status=OL`, 100% battery charge, and 25,650 seconds runtime.
- Three subsequent diagnostic samples remained fresh and healthy with the same
  model, status, charge, and runtime. Board uptime advanced throughout; there
  was no reboot, flash, manual driver restart, or user wait beyond the cable
  move.
- Read-only NUT `LIST UPS` returned the `cyberpower` service. `LIST VAR`
  returned 54 variables, including `ups.mfr=CPS`, `ups.model=CST150UC2`,
  `driver.name=usbhid-ups`, and `ups.status=OL`. `GET VAR cyberpower ups.status`
  returned `VAR cyberpower ups.status "OL"`.

### CyberPower CST150UC2 to APC Back-UPS RS 1500G

- The first post-swap diagnostic samples showed NUT stale and UPS fields
  unavailable. Logs showed HID connect/start and selection of the APC HID
  subdriver `0.100`.
- A later poll cleared stale state and returned model `Back-UPS RS 1500G`,
  `ups.status=OL`, 100% battery charge, and 4,518 seconds runtime. Board uptime
  advanced throughout; there was no reboot, flash, or manual driver restart.
- Read-only NUT `LIST UPS` returned the `cyberpower` service. `LIST VAR`
  returned 50 variables, including `ups.mfr=American Power Conversion`,
  `ups.model=Back-UPS RS 1500G`, `driver.name=usbhid-ups`,
  `ups.firmware=865.L3 .D`, and `ups.status=OL`. `GET VAR cyberpower ups.status`
  returned `VAR cyberpower ups.status "OL"`.

## Assessment

- **Observed:** Both exact models recovered automatically after physical USB
  replacement on the reported current target firmware. Both initially showed
  stale state with unavailable UPS fields while the HID driver probed, then
  produced fresh, model-correct `OL` polls. No reboot or firmware operation
  occurred during either replacement.
- **Inferred:** The brief stale responses are the normal replacement-probe
  window implemented by the existing USB HID reprobe path. This test does not
  explain the earlier multi-day APC stale condition reported by the Maintainer;
  that condition was not reproduced after the Maintainer's reimage/reset.
- **Not tested:** Other APC/CyberPower models, non-HID UPS protocols, external
  `upsc` clients, and sustained operation beyond the repeated short samples.

## Commands and access

Diagnostics used the fingerprint-pinned Agent status probe with the existing
1Password environment. Direct NUT validation used only read-only `LIST` and
`GET` requests on port 3493. Secret values and network coordinates are omitted.
