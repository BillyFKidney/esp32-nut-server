# ESP32 Port of NUT (Network UPS Tools)

## Overview

This is an ESP32 port of the Network UPS Tools (NUT). The current downstream
milestone provides read-only USB HID UPS monitoring and NUT network access on
ESP32-S3. UPS-control capabilities inherited from the alpha port are disabled.

See [ESP32_DEVELOPMENT_PLAN.md](ESP32_DEVELOPMENT_PLAN.md) for the downstream
roadmap, completed milestones, and feature tracking.

## Features

- USB HID UPS support via ESP32 USB Host
- Wi-Fi provisioning with station mode and a fallback captive portal
- Development OTA firmware uploads over the connected Wi-Fi network
- NUT `usbhid-ups` driver running on ESP32
- Read-only NUT `upsd` server on TCP port 3493
- CyberPower HID subdriver support
- Web-based monitoring (when configured)
- Persistent configuration storage using FAT filesystem

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
. $HOME/esp/esp-idf/export.sh

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

The portal saves submitted credentials as *pending*, then restarts the device
to test them in station-only mode. A valid DHCP address promotes them to the
saved configuration. If association or DHCP fails, the pending credentials are
retained for automatic retries after future restarts and the setup access point
returns. Submitting new credentials replaces the pending values; holding
**BOOT** for three seconds during startup erases all saved Wi-Fi credentials.
This avoids testing a new network while the captive AP is active on the same
radio.

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
erase NUT or firmware data.

### Development OTA updates

The retained partition table already contains two 3.3 MB OTA application slots.
When the station interface is connected, ESP32-NUT starts a development-only
OTA server on TCP port `8080`. Its status endpoint is:

```
http://<esp32-ip>:8080/
```

Upload a complete ESP-IDF application image to `POST /ota`; for example, from
the development Mac:

```
curl --fail --data-binary @build/nut-esp32s3.bin \
  http://<esp32-ip>:8080/ota
```

The server writes only to the inactive OTA slot, verifies the image, selects
it for the next boot, then restarts the ESP32. Bootloader rollback is enabled:
a new image that fails before core services start returns to the prior image.

**This endpoint is intentionally unauthenticated for development only.** Do
not expose TCP port `8080` outside a trusted LAN. Production OTA must add TLS,
firmware signing, and an authenticated update policy before it is enabled.

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
   - No SSL/TLS support in current implementation
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
