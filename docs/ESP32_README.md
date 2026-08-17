# ESP32-NUT downstream notes

The application landing page is [README.md](../README.md). Full v2.7.1 port,
architecture, compatibility, troubleshooting, and changelog notes are retained
in [archive/ESP32_README_V2_7_1.md](archive/ESP32_README_V2_7_1.md).

## Current target and boundaries

ESP32-NUT targets YD-ESP32-23 / ESP32-S3-WROOM-1-N16R8 with ESP-IDF v6.0.2,
16 MB DIO flash, and 8 MB octal PSRAM. It provides LAN-only HTTPS management
on `443` and read-only NUT on `3493`; retired unauthenticated `8080` remains
refused. UPS controls remain disabled.

The board's native USB connector may lack VBUS when its USB-OTG jumper is open.
Verify the exact board revision before bridging it; do not create a USB 5 V
backfeed path. Use [ESP32_PREFLIGHT.md](ESP32_PREFLIGHT.md) before any
hardware, flash, or serial action.

## Build

```bash
. /Users/billyfkidney/.espressif/v6.0.2/esp-idf/export.sh
idf.py build
```

Generated `build/`, `managed_components/`, `dependencies.lock`, and local
`sdkconfig` remain untracked. Flashing and OTA installation require explicit
authority; the release image is an application image for the authenticated OTA
workflow.

## Configuration and diagnosis

Wi-Fi credentials are configured at runtime and are never compiled into the
firmware. NUT is read-only and configures no NUT users. For current branch,
device, and acceptance facts, read [ESP32_CURRENT_STATUS.md](ESP32_CURRENT_STATUS.md).
For detailed recovery, transport, and security information, use the linked
preflight, security, proxy, and archived documents rather than guessing.
