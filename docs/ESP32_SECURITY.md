# ESP32 Security Considerations for NUT

## Overview

This document outlines security considerations and best practices for deploying NUT (Network UPS Tools) on ESP32 platforms.

## Critical Security Issues

### 1. Credentials

**⚠️ CRITICAL: Wi-Fi provisioning is intentionally open by project policy.**

ESP32-NUT does not include hardcoded Wi-Fi credentials. It stores submitted
credentials as pending in NVS, then promotes them after a station-only
connection test.

#### UPS Daemon Credentials
- **Location**: `/usr/local/etc/nut/upsd.users`
- **Default**: No users are configured in the read-only milestone
- **Behavior**: Unauthenticated `GET` and `LIST` requests are available;
  login-dependent `SET`, `INSTCMD`, and `FSD` requests cannot be authorized
- **If users are enabled later**: Use unique passwords, grant only required
  permissions, and restrict access to the configuration file

#### Wi-Fi Credentials
- **Location**: NVS namespace `wifi-config` on the device
- **Default**: No network is configured
- **Setup behavior**: If no saved network connects, the device starts an open
  access point and an unauthenticated HTTP captive portal at `192.168.4.1`.
- **Risk**: Anyone within radio range can join the setup network while it is
  active, observe portal traffic, and submit different network credentials.
  Wi-Fi passwords are exposed on the open setup link and stored in NVS without
  flash encryption unless the device owner enables platform security features.
- **Actions Required**:
  1. Provision from a trusted physical location and complete it promptly.
  2. Use WPA2/WPA3 on the target Wi-Fi network.
  3. Hold **BOOT** for three seconds during startup to erase Wi-Fi credentials
     before transferring or decommissioning a device.
  4. Enable flash encryption and secure boot before a hostile-environment
     deployment; these are not enabled by this project by default.

### 2. File Permissions

The current implementation uses relaxed file permissions that have been improved but should still be reviewed:

- Configuration directories: `0755` (owner: rwx, group/others: rx)
- Configuration files: Should be set to `0600` or `0640` for sensitive files

**Recommended Actions**:
1. Review and restrict permissions on all configuration files
2. Ensure sensitive files are not world-readable
3. Implement proper user/group ownership when possible

### 3. Network Security

#### Default Network Configuration
- **Wi-Fi setup mode**: Creates an open access point only when setup or
  recovery is required
- **UPS Server**: Listens on `0.0.0.0:3493` (all interfaces)

**Recommended Actions**:
1. Restrict physical proximity while the open setup portal is active
2. Implement network access controls
3. Consider using TLS/SSL for UPS server connections
4. Restrict `LISTEN` directive to specific interfaces if possible

## Security Hardening Checklist

### Before Deployment

- [ ] Provision Wi-Fi from a trusted location
- [ ] Keep NUT users disabled unless authenticated operations are required
- [ ] Review and restrict file permissions
- [ ] Enable WPA2/WPA3 WiFi encryption
- [ ] Configure network access controls
- [ ] Review and limit exposed services
- [ ] Test security configuration

### Runtime Security

- [ ] Monitor authentication logs
- [ ] Regularly update firmware
- [ ] Audit configuration changes
- [ ] Implement rate limiting for authentication attempts
- [ ] Use secure channels for remote access

### Network Configuration

- [ ] Use strong WiFi encryption (WPA3 preferred)
- [ ] Implement MAC address filtering if needed
- [ ] Use VLANs to isolate UPS management traffic
- [ ] Configure firewall rules
- [ ] Disable unused network services

## Secure Configuration Examples

### Example: Secure upsd.users Configuration

```ini
# /usr/local/etc/nut/upsd.users
[admin]
  password = <USE_STRONG_PASSWORD_HERE>
  actions = SET
  instcmds = ALL
  
[monitor]
  password = <USE_DIFFERENT_STRONG_PASSWORD>
  upsmon primary
```

### Example: Restricted upsd.conf

```ini
# /usr/local/etc/nut/upsd.conf
LISTEN 192.168.1.100 3493  # Listen only on specific interface
MAXCONN 4
MAXAGE 15
```

## Known Limitations

### Platform Stubs

The ESP32 port includes stub implementations for several POSIX security functions:

- `sigaction()`, `signal()` - Signal handling not implemented
- `fchmod()`, `fchown()`, `chown()` - Permission changes are no-ops
- `setuid()`, `setgid()`, `seteuid()` - Privilege changes are no-ops
- `chroot()` - Chroot is not implemented

**Implication**: Traditional UNIX security mechanisms are not available. Implement alternative security controls at the network and application level.

### Resource Constraints

ESP32 devices have limited resources:
- RAM: ~520KB available
- Flash: Varies by module
- CPU: Dual-core 240MHz

**Recommendations**:
- Monitor memory usage
- Limit number of concurrent connections
- Implement watchdog timers
- Use appropriate task priorities

## Secure Development Practices

### Code Review
- Review all ESP32-specific code for security issues
- Use static analysis tools (cppcheck, clang-tidy)
- Conduct security audits before major releases

### Testing
- Test authentication mechanisms
- Verify access controls
- Test resource exhaustion scenarios
- Validate input handling

### Updates
- Implement secure firmware update mechanism
- Sign firmware images
- Verify signatures before updates
- Maintain rollback capability

## Incident Response

### If Compromise is Suspected

1. **Immediate Actions**:
   - Disconnect device from network
   - Document observed behavior
   - Preserve logs if available

2. **Investigation**:
   - Review authentication logs
   - Check for configuration changes
   - Analyze network traffic

3. **Recovery**:
   - Change all credentials
   - Update firmware
   - Review and harden configuration
   - Reconnect to network with monitoring

## Additional Resources

- [ESP32 Security Features](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/security/index.html)
- [NUT Security Documentation](https://networkupstools.org/docs/user-manual.chunked/ar01s06.html)
- [WiFi Security Best Practices](https://www.wi-fi.org/discover-wi-fi/security)

## Reporting Security Issues

If you discover a security vulnerability in the ESP32 port of NUT:

1. **Do not** open a public issue
2. Email the security contact (see SECURITY.md in project root)
3. Include:
   - Description of the vulnerability
   - Steps to reproduce
   - Potential impact
   - Suggested fix (if available)

## License and Disclaimer

This security documentation is provided as-is without warranty. Users are responsible for properly securing their deployments according to their specific requirements and threat models.
