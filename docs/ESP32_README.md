# ESP32 Port of NUT (Network UPS Tools)

> The application landing page is the root [README.md](../README.md). This
> document keeps detailed downstream port notes; current device facts belong in
> [ESP32_CURRENT_STATUS.md](ESP32_CURRENT_STATUS.md), and the current
> Synology/AdGuard browser path is documented in
> [ESP32_MANAGEMENT_PROXY.md](ESP32_MANAGEMENT_PROXY.md).

## Overview

This is an ESP32 port of the Network UPS Tools (NUT). The current downstream
milestone provides read-only USB HID UPS monitoring and NUT network access on
ESP32-S3. UPS-control capabilities inherited from the alpha port are disabled.

See [ESP32_DEVELOPMENT_PLAN.md](ESP32_DEVELOPMENT_PLAN.md) for the downstream
roadmap, completed milestones, and feature tracking.

See [ESP32_CURRENT_STATUS.md](ESP32_CURRENT_STATUS.md) for the active branch,
installed-firmware evidence, and exact next action. Before hardware work, use
[ESP32_PREFLIGHT.md](ESP32_PREFLIGHT.md); team authority and responsibilities
are defined in [ESP32_DEVELOPMENT_ROLES.md](ESP32_DEVELOPMENT_ROLES.md).

## Features

- USB HID UPS support via ESP32 USB Host
- Wi-Fi provisioning with station mode and a fallback captive portal
- Authenticated local firmware checking and installation over HTTPS
- NUT `usbhid-ups` driver running on ESP32
- Read-only NUT `upsd` server on TCP port 3493
- CyberPower HID subdriver support
- Authenticated HTTPS administration and dashboard diagnostics
- Persistent Wi-Fi and management configuration in device storage

## Hardware Requirements

- ESP32-S3 DevKit C-1 (or compatible ESP32 board with USB host support)
- USB cable for connecting UPS
- Power supply for ESP32
- Compatible UPS device (currently tested with CyberPower CST150UC2)

### Squirrel Powered Labs target

The downstream ESP32-NUT build currently targets:

- YD-ESP32-23 development board
- ESP32-S3-WROOM-1-N16R8 module
- 16 MB quad-SPI flash in DIO mode at 80 MHz
- 8 MB octal PSRAM at 80 MHz
- Direct USB Serial/JTAG development connection
- Native USB host connection to a USB HID UPS

The inherited `nut_fat_8MB.csv` partition table is intentionally retained for
the first ESP-IDF 6.0.2 build-and-boot milestone. It uses only the lower 8 MB
of the available flash; expanding the storage layout is a separate change.

On YD-ESP32-23 boards, the native connector's D+/D- lines are wired to the
ESP32-S3 but its VBUS is not supplied when the `USB-OTG` solder jumper is open.
The jumper must be verified against the exact board revision before bridging
it. Closing it ties the two connector VBUS rails together, so external 5 V
sources must not be connected in a way that can backfeed either USB host.

## Software Requirements

- ESP-IDF v6.0.2 (validated downstream version)
- PlatformIO (optional, for easier building)
- Python 3.x (for ESP-IDF tools)

## Building

### Using PlatformIO

```bash
# Install PlatformIO if not already installed
pip install platformio

# Build for ESP32-S3
pio run -e esp32-s3-devkitc-1

# Upload to device
pio run -e esp32-s3-devkitc-1 -t upload

# Monitor serial output
pio device monitor
```

### Using ESP-IDF

```bash
# Set up ESP-IDF environment
. /Users/billyfkidney/.espressif/v6.0.2/esp-idf/export.sh

# Configure project
idf.py set-target esp32s3
idf.py menuconfig

# Build
idf.py build

# Flash
idf.py -p /dev/ttyUSB0 flash monitor
```

This downstream tree is validated with ESP-IDF v6.0.2. The first local build
creates `sdkconfig`, `dependencies.lock`, `managed_components/`, and `build/`;
all are generated and remain untracked. Portable target settings live in
`sdkconfig.defaults`.

The current downstream milestone runs USB discovery and the NUT HID driver in
read-only mode. Discovery prints connect/disconnect events and the device,
configuration, interface, endpoint, and cached string descriptors. The driver
uses explicit CyberPower VID/PID matching and polling; its ESP USB backend
blocks HID writes and returns report-read failures to NUT instead of aborting
ESP-IDF. The NUT network server exposes read-only `GET` and `LIST` requests;
no authenticated NUT users are configured.

## Configuration

### WiFi Configuration

Wi-Fi credentials are not compiled into the firmware. On a new device, or when
saved credentials cannot connect, ESP32-NUT starts an open setup access point
named `ESP32-NUT-<MAC suffix>` at `192.168.4.1`. Connect to it and use the
captive portal to select or enter a Wi-Fi network and its password.

The portal stages submitted credentials, then restarts the device to test them
in station-only mode. A valid DHCP address promotes them to the saved
configuration. If association or DHCP fails, the previously active
credentials remain in place, the failed pending state is cleared, and the setup
access point returns. Submitting new credentials replaces the pending values;
holding **BOOT** for three seconds during startup erases all saved Wi-Fi
credentials. This avoids testing a new network while the captive AP is active
on the same radio.

### DHCP compatibility

The ESP-IDF client-side ARP probe for a newly offered DHCP address is disabled
in the project defaults. On the validated UniFi network, the DHCP server ACKed
the ESP32 address but that probe falsely rejected the lease. The device still
uses DHCP; do not assign it a conflicting static IPv4 address.

The setup access point and portal are intentionally unauthenticated by project
policy. Complete setup promptly, and do not use them where untrusted people can
join the setup network. The password reveal control only changes the local
browser display; it does not log the password.

To erase the saved Wi-Fi configuration, hold the board's **BOOT** button for
three seconds during startup. This only clears Wi-Fi configuration; it does not
erase NUT or firmware data. Continue holding through fifteen seconds to perform
a factory reset: Wi-Fi, administrator credentials, API credentials, device
identity, and the device HTTPS certificate are erased, while the current
firmware and OTA recovery slot remain bootable.

### Operational Management HTTPS transition

The `feature/operational-management` branch replaced the development HTTP OTA
listener with a LAN-only HTTPS administration service on TCP port `443`. The
ESP32 creates and stores a unique self-signed certificate and private key. When
the board is accessed directly by IP, the browser will show a certificate
warning. The current local deployment normally reaches the board through the
Synology reverse proxy, which presents the trusted wildcard certificate to the
browser and uses HTTPS to the ESP32 backend. See
[ESP32_MANAGEMENT_PROXY.md](ESP32_MANAGEMENT_PROXY.md).

The first administration page requires the owner to create the per-device
ADMIN password twice. Passwords are stored only as salted PBKDF2-HMAC-SHA-256
verifiers. Authenticated browser sessions use Secure, HttpOnly, SameSite cookies
and expire after fifteen minutes of idle time. The HTTPS management API also
uses CSRF protection and throttles repeated failed password attempts.

ADMIN can create up to four named, non-expiring API tokens. A complete token is
shown exactly once; later lists show only its name, device-generated UTC issue
date, and final four characters. Token deletion requires an acknowledgement
checkbox and explicit confirmation. In `v2.3.0`, each token is limited to the
Bearer-authenticated `ota.install` route for Agent-driven firmware upload and
cannot authenticate token management or other ADMIN routes. See
[ESP32_SECURITY.md](ESP32_SECURITY.md) for the exact route boundaries and safe
client workflow.

The administration console shows device UTC/local time and whether the clock is
unavailable, retained, manually set, waiting for NTP, or synchronized. Automatic
SNTP defaults to `pool.ntp.org`; ADMIN may configure the hostname, request an
immediate synchronization, disable NTP, select a supported IANA time zone, or
set local date/time manually. The default time zone is
`America/Los_Angeles`. Supported selections are `UTC`, the principal United
States continental zones, `America/Phoenix`, `America/Anchorage`, and
`Pacific/Honolulu`. An unset clock is never presented as a valid 1970 date.

The existing dual 3.3 MB OTA slots and bootloader rollback behavior remain in
place. Firmware upload uses the authenticated HTTPS management API and the
scoped Agent OTA route described above; do not rely on the former
unauthenticated TCP port `8080` service.

The `v2.5.0` administration update keeps this same authenticated page
but organizes it with a responsive tab bar: **Dashboard**, **Device Status**,
**Date and Time**, **Wi-Fi Configuration**, **ADMIN Password**, **API Tokens**,
and **Update Firmware**. The tabs are presentation-only and do not change
route authorization or transport security.

The Wi-Fi Configuration panel provides supported-network scanning
through a visible selectable list containing each SSID, signal strength, and
security mode. Manual SSID entry remains available for hidden or unlisted
networks. The panel also provides safe credential staging and reconnect, plus
a password field that is masked by default. Its local **Show password** toggle
changes only the current browser control; the stored password is never
returned, persisted by the UI, or logged.

The `v2.6.0` local OTA-management slice adds an ADMIN-session and CSRF-
protected **Check firmware** action before installation. The device validates
the complete local ESP32 image without selecting a boot slot, rejects invalid
images, and continues to use the existing authenticated install and scoped
Agent OTA routes. The v2.6 candidate passed target rollback/persistence
validation on the `.173` development device; the device never downloads
firmware from a remote source.

HTTPS does not add TLS to the read-only NUT service on TCP port `3493`; keep
that service within a trusted management network.

### UPS Configuration

After the device boots, configuration files are automatically created in the virtual filesystem:

- `/usr/local/etc/nut/ups.conf` - UPS device configuration
- `/usr/local/etc/nut/upsd.conf` - Server configuration
- `/usr/local/etc/nut/upsd.users` - User authentication (empty by default)

The read-only milestone does not configure NUT user accounts. NUT `GET` and
`LIST` requests do not require authentication. Login-dependent operations such
as `SET`, `INSTCMD`, and `FSD` cannot be authorized.

### Network Access

The NUT server listens on:
- IP: `0.0.0.0` (all interfaces)
- Port: `3493`

Access the server using any NUT client:
```bash
upsc cyberpower@<esp32-ip-address>
```

## Security Considerations

**⚠️ CRITICAL: Read the security documentation before production deployment!**

See [ESP32_SECURITY.md](ESP32_SECURITY.md) for detailed security considerations including:

- Open setup portal exposure and stored Wi-Fi credentials
- File permission hardening
- Network security configuration
- Secure deployment checklist

### Quick Security Checklist

Before production deployment:
- [ ] Provision Wi-Fi from a trusted location
- [ ] Keep `upsd.users` empty unless authenticated operations are required
- [ ] Review file permissions on configuration files
- [ ] Configure network access controls
- [ ] Enable WPA2/WPA3 WiFi encryption
- [ ] Limit UPS server to specific network interfaces
- [ ] Test authentication and access controls

## Architecture

### Components

```
ESP32 Application (app_main)
├── Wi-Fi Provisioning (wifi_provisioning_init)
├── USB Host Stack (hidHostInstall)
│   └── HID Device Driver
├── Filesystem (mountFS)
│   ├── /var - Runtime data
│   └── /usr - Configuration files
├── NUT Driver Task (drv_main)
│   └── usbhid-ups with CyberPower HID subdriver
└── NUT Server Task (nut_main)
    └── upsd server
```

### Memory Layout

- Flash: Partitioned for code, data, and FAT filesystem
- RAM: ~520KB available for tasks and data structures
- Task Stack Sizes:
  - USB tasks: 4KB
  - Driver task: 16KB
  - Server task: 16KB

### Threading Model

The application uses FreeRTOS tasks:

1. **USB Library Task**: Handles USB host events
2. **HID Background Task**: Processes HID device events
3. **Driver Task** (drv_main): Runs the UPS driver
4. **Server Task** (nut_main): Serves read-only NUT network requests
5. **Main Task**: Monitors system state

## File Structure

```
src/
├── main.c              - ESP32 application entry point
├── usb.c               - USB host implementation
├── wifi.c              - WiFi configuration
├── drivers/
│   ├── espusb.c        - ESP32 USB driver backend
│   └── espusb.h        - USB driver header
└── [other NUT files]   - Standard NUT components

docs/
└── ESP32_SECURITY.md   - Security documentation
```

## Supported UPS Devices

Currently tested and supported:

- CyberPower CST150UC2 (`0764:0601`, USB HID, `pollonly`)

The inherited ESP32 branch included a CyberPower header but omitted the
corresponding `cps-hid.c` implementation and disabled the CyberPower subdriver.
This downstream restores `cps-hid.c` from upstream NUT commit `2dce981e`, the
exact merge base of the ESP32 port, and enables its subdriver table entry. The
driver reads standard CyberPower telemetry through the existing NUT HID stack.

### Hardware validation

The following behavior was validated on the Squirrel Powered Labs target with
ESP-IDF v6.0.2 and a CyberPower CST150UC2:

- Cold boot with the UPS absent and with it already attached
- VID, PID, manufacturer, model, serial, configuration, interface, and endpoint
  discovery
- CyberPower HID report-descriptor parsing and regular two-second polling
- Battery capacity, runtime, voltage, load, power, and `OL` status reporting
- Clean disconnect, `DATASTALE`, re-enumeration, reconnect, and resumed full
  polling without a reboot
- SoftAP reachability at `192.168.4.1` and NUT service on TCP port 3493
- `LIST UPS` and live `GET VAR` responses from a network client
- Rejection of the removed legacy credential with `ERR ACCESS-DENIED`
- `ERR DATA-STALE` over the network after disconnect and resumed live telemetry
  after reconnect, without restarting `upsd`

The ESP USB backend performs control `GET_REPORT` transfers through the raw USB
Host Library client. Receive buffers are rounded for the ESP32-S3 endpoint-zero
DMA requirements while the HID request length remains exact. This avoids the
ESP-IDF HID host component assertion seen with exact 64-byte reports.

For a complete list of supported devices, see the [NUT Hardware Compatibility List](https://networkupstools.org/stable-hcl.html).

## Known Limitations

### Platform Limitations

1. **POSIX Stubs**: Many POSIX functions are stubbed and don't perform actual operations:
   - User/group management (setuid, setgid, etc.)
   - Signal handling (sigaction, signal)
   - File permissions (fchmod, fchown)

2. **Resource Constraints**:
   - Limited RAM (~520KB)
   - Limited number of concurrent connections (4 max)
   - No fork/exec support

3. **Network**:
   - The Operational Management branch provides HTTPS for the administration
     console; the read-only NUT service itself does not yet support TLS.
   - Single network interface

### Functional Limitations

1. **Read-only USB**: HID `SET_REPORT` is blocked. UPS commands and writable
   variables are not supported by this milestone.
2. **Polling only**: The ESP USB backend's interrupt-transfer and string-read
   entry points remain stubs; `pollonly` is required.
3. **Vendor-private reports**: CyberPower reports `0x28` and `0x29` currently
   return an invalid-response error when the host controller reports padded
   bytes beyond the requested length. These reports contain vendor-private
   usages not mapped into the standard telemetry validated above.
4. **Read-only network server**: `upsd` serves unauthenticated `GET` and `LIST`
   requests. No authenticated users or TLS support are configured.
5. **USB scope**: Only HID devices are currently supported.
6. **Filesystem**: FAT filesystem with wear leveling.
7. **Configuration**: No runtime configuration UI.

## Troubleshooting

### Common Issues

#### Device Not Detected

```
ESP_LOGE: No device attached
```

**Solutions**:
- Verify USB cable is properly connected
- Check that UPS is powered on
- Ensure ESP32 has sufficient power supply
- Verify UPS is HID-compatible

#### WiFi Connection Failed

```
ESP_LOGI: Failed to connect to SSID:...
```

**Solutions**:
- Verify SSID and password are correct
- Check WiFi signal strength
- Ensure router supports ESP32 WiFi standard
- Try different WiFi channel

#### Memory Allocation Failed

```
ESP_LOGE: Failed to allocate memory
```

**Solutions**:
- Reduce MAXCONN in upsd.conf
- Increase heap size in menuconfig
- Close unnecessary tasks/connections

### Debug Logging

Enable verbose logging by setting debug level in `main.c`:

```c
nut_debug_level = 9;  // Maximum verbosity
```

Monitor serial output:
```bash
pio device monitor
# or
idf.py monitor
```

## Performance Tuning

### Task Priorities

Adjust task priorities in `main.c` if needed:
```c
xTaskCreatePinnedToCore(drv_main, "drv_main", 8192 * 2, NULL, 5, NULL, 0);
xTaskCreatePinnedToCore(nut_main, "nut_main", 8192 * 2, NULL, 5, NULL, 0);
```

### Watchdog Timeout

Adjust watchdog timeout for slower operations:
```c
esp_task_wdt_config_t twdt_config = {
    .timeout_ms = 60000,  // 60 seconds
    // ...
};
```

### Connection Limits

Reduce MAXCONN in `/usr/local/etc/nut/upsd.conf`:
```ini
MAXCONN 2  # Default is 4
```

## Development

### Adding New Drivers

1. Implement driver in `src/drivers/`
2. Update `drivers_main()` in `main.c`
3. Add configuration to `ups.conf` generation

### Debugging

Use ESP-IDF debugging tools:
```bash
# OpenOCD with ESP-PROG
idf.py openocd

# GDB
xtensa-esp32s3-elf-gdb build/nut.elf
```

### Testing

Validate the driver over the serial console and query `upsd` over the network.
A complete test includes cold boot, live `GET VAR` responses, disconnect to
`ERR DATA-STALE`, reconnect, and resumed live responses without a reset.

## Contributing

Contributions are welcome! Please:

1. Follow existing code style
2. Test on actual hardware
3. Update documentation
4. Consider security implications
5. Submit pull requests to the main NUT repository

## License

This ESP32 port follows the same license as the main NUT project (GPL-2.0+).

See [LICENSE](../LICENSE-GPL2) for details.

## References

- [NUT Documentation](https://networkupstools.org/documentation.html)
- [ESP-IDF Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/)
- [ESP32 USB Host](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/usb_host.html)
- [ESP32 Security Features](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/security/index.html)

## Support

For ESP32-specific issues:
- Check troubleshooting section above
- Review ESP32_SECURITY.md
- Open an issue on GitHub with "ESP32:" prefix

For general NUT questions:
- Visit [NUT support page](https://networkupstools.org/support.html)
- Join NUT mailing lists
- Consult NUT documentation

## Changelog

### Current Version

- ESP-IDF v6.0.2 and ESP32-S3-N16R8 build support
- Read-only USB discovery and CyberPower NUT HID polling
- Endpoint-zero control-transfer workaround for 64-byte reports
- Clean USB disconnect and hot reconnect handling

See git history for detailed changes.
