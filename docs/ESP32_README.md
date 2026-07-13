# ESP32 Port of NUT (Network UPS Tools)

## Overview

This is an ESP32 port of the Network UPS Tools (NUT), enabling UPS monitoring and management on ESP32-based devices. The port allows ESP32 microcontrollers to act as UPS servers, providing status information and control capabilities over WiFi.

## Features

- USB HID UPS support via ESP32 USB Host
- WiFi connectivity (Station and Access Point modes)
- NUT server (upsd) running on ESP32
- APC HID driver support
- Web-based monitoring (when configured)
- Persistent configuration storage using FAT filesystem

## Hardware Requirements

- ESP32-S3 DevKit C-1 (or compatible ESP32 board with USB host support)
- USB cable for connecting UPS
- Power supply for ESP32
- Compatible UPS device (tested with APC HID devices)

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

- ESP-IDF v5.4.0 or later
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

The current downstream milestone runs in read-only USB discovery mode. It
waits for connect and disconnect events and prints the device, configuration,
interface, endpoint, and cached string descriptors. NUT driver/server startup
is paused until discovery is successful, preventing an absent device from
calling `exit()` and rebooting ESP-IDF.

## Configuration

### WiFi Configuration

**⚠️ SECURITY WARNING**: Default WiFi credentials are hardcoded!

Before deployment, modify the WiFi credentials in `src/wifi.c`:

```c
#define EXAMPLE_ESP_WIFI_SSID "your-ssid-here"
#define EXAMPLE_ESP_WIFI_PASS "your-password-here"
```

**Recommended**: Implement WiFi provisioning using BLE, WPS, or a web interface.

### UPS Configuration

After the device boots, configuration files are automatically created in the virtual filesystem:

- `/usr/local/etc/nut/ups.conf` - UPS device configuration
- `/usr/local/etc/nut/upsd.conf` - Server configuration
- `/usr/local/etc/nut/upsd.users` - User authentication

**⚠️ SECURITY WARNING**: Default passwords are set!

Default credentials (MUST be changed for production):
- User `nut`: password `espdonut`
- User `monuser`: password `pass`

### Network Access

The NUT server listens on:
- IP: `0.0.0.0` (all interfaces)
- Port: `3493`

Access the server using any NUT client:
```bash
upsc ups@<esp32-ip-address>
```

## Security Considerations

**⚠️ CRITICAL: Read the security documentation before production deployment!**

See [ESP32_SECURITY.md](ESP32_SECURITY.md) for detailed security considerations including:

- Changing default credentials
- File permission hardening
- Network security configuration
- Secure deployment checklist

### Quick Security Checklist

Before production deployment:
- [ ] Change WiFi SSID and password
- [ ] Change all UPS daemon user passwords
- [ ] Review file permissions on configuration files
- [ ] Configure network access controls
- [ ] Enable WPA2/WPA3 WiFi encryption
- [ ] Limit UPS server to specific network interfaces
- [ ] Test authentication and access controls

## Architecture

### Components

```
ESP32 Application (app_main)
├── WiFi Configuration (wifi_init_softap/sta)
├── USB Host Stack (hidHostInstall)
│   └── HID Device Driver
├── Filesystem (mountFS)
│   ├── /var - Runtime data
│   └── /usr - Configuration files
├── NUT Driver Task (drv_main)
│   └── apc-hid driver
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
4. **Server Task** (nut_main): Runs the upsd server
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
- APC Back-UPS (USB HID interface)

The inherited ESP32 branch included a CyberPower header but omitted the
corresponding `cps-hid.c` implementation and disabled the CyberPower subdriver.
This downstream restores `cps-hid.c` from upstream NUT commit `2dce981e`, the
exact merge base of the ESP32 port, and enables its subdriver table entry. Raw
USB discovery is validated; NUT runtime communication remains a separate
milestone.

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

1. **USB Support**: Only HID devices currently supported
2. **Filesystem**: FAT filesystem with wear leveling
3. **Configuration**: No runtime configuration UI

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

Test UPS communication:
```bash
# From another machine on the network
upsc ups@<esp32-ip>

# List available commands
upscmd -l ups@<esp32-ip>
```

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
- Initial ESP32 port
- Security improvements and documentation
- Buffer overflow fixes
- Memory leak fixes

See git history for detailed changes.
