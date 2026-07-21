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

#### HTTPS administration and OTA transition

The Operational Management branch removes the unauthenticated development OTA
endpoint on TCP port `8080`. When station Wi-Fi receives an IPv4 address, the
device starts a LAN-only HTTPS administration service on TCP port `443`.

- A unique self-signed certificate and private key are generated on first use
  and stored in the management NVS namespace until factory reset.
- Initial setup requires the owner to choose the ADMIN password twice. The
  device retains only a salted PBKDF2-HMAC-SHA-256 verifier.
- Browser sessions use Secure, HttpOnly, SameSite cookies, expire after a
  fifteen-minute idle period, and state-changing browser requests require a
  CSRF header.
- Password login attempts are throttled after repeated failures.

#### API-token and OTA authorization boundaries

ESP32-NUT supports at most four active, uniquely named API tokens. Each token
contains 256 bits of device-generated randomness and is returned only in the
successful creation response. The device retains a random salt, a SHA-256
salted verifier, the name, device-generated UTC issue date, final four
characters, public identifier, and explicit scope bits. It never retains or
lists the complete token. The versioned NVS representation is one fixed
456-byte blob in the existing 20 KiB management partition: a short header and
four fixed 112-byte records. An unknown version, record size, or invalid record
fails closed instead of being interpreted or silently migrated. There is no
legacy token state to migrate into v2.3.0.

All v2.3.0 tokens have only the `ota.install` scope. Send a token in the HTTPS
`Authorization` header using the Bearer scheme; query parameters and cookies
are not accepted as API-token credentials. Verifiers are compared in constant
time. The route boundaries are:

| Route | Method | Authorization | Boundary |
| --- | --- | --- | --- |
| `/api/v1/admin/tokens` | `GET` | ADMIN session | List non-secret token metadata only |
| `/api/v1/admin/tokens` | `POST` | ADMIN session and CSRF | Create a token and disclose it once |
| `/api/v1/admin/tokens` | `DELETE` | ADMIN session, CSRF, and acknowledgement | Permanently revoke one token |
| `/api/v1/ota/install` | `POST` | ADMIN session and CSRF | Preserve authenticated Safari OTA |
| `/api/v1/agent/ota/install` | `POST` | Bearer token with `ota.install` | Accept only a raw ESP-IDF application image with `Content-Type: application/octet-stream` for verified OTA installation |

API tokens do not authorize browser pages, status, password changes, time
configuration, token management, logout, or future management routes. A token
is non-expiring until ADMIN deletes it or the fifteen-second physical factory
reset erases the management NVS namespace. Token issue time is display metadata
and never an authorization or expiration input.

For Agent-driven OTA, place the token privately in the
`ESP32_NUT_OTA_TOKEN` environment variable and run:

```bash
python3 tools/esp32-agent-ota.py \
  --device <current-esp32-ip> \
  --firmware build/nut-esp32s3.bin \
  --certificate-sha256 <trusted-device-certificate-sha256>
```

The helper never accepts a token on its command line and does not print the
Authorization header. Pin the fingerprint of the v2.x self-signed certificate
after confirming it through an already trusted browser, or provide a trusted
PEM with `--ca-file`. The explicit `--insecure-self-signed` mode is limited to
temporary diagnostics because it does not authenticate the peer and can expose
the Bearer token to a LAN attacker. Unset the environment variable after use.
If a token may have been disclosed, delete it in the ADMIN console and create a
replacement rather than attempting to recover it.

Use `tools/esp32-api-token-probe.py` for small authorization-boundary tests.
It accepts only the explicitly allowlisted status, token-management, and Agent
OTA paths; verifies the configured certificate fingerprint on the same TLS
connection before sending Authorization; reads the token only from the same
environment variable; and redacts token-shaped response text. Do not use
`curl -k` with a valid token: the device's self-signed v2.x certificate has no
subjectAltName extension, so ordinary IP-hostname verification is unavailable.

The authenticated console also manages device time. Automatic SNTP uses
`pool.ntp.org` by default after station Wi-Fi receives an address. ADMIN may
select a supported IANA time-zone name, configure the NTP hostname, disable or
restart NTP, or set local date/time manually when NTP is unavailable. The
device stores configuration but does not persist a manually entered clock value
across power loss; it reports an unavailable clock explicitly until retained or
newly synchronized time is valid.

Public SNTP does not authenticate the time source. ESP32-NUT therefore uses
wall-clock time for display and operational metadata, not as an authorization
decision, API-token expiration mechanism, or firmware-trust signal. Manual time
is replaced if a later SNTP synchronization succeeds.

The certificate is self-signed for Milestone 2, so browsers will show a trust
warning until the owner explicitly accepts it. Milestone 3 replaces it with a
certificate issued by the local CA and adds production OTA source/signature
verification. HTTPS protects the administration interface, not the read-only
NUT service on TCP port `3493`.

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
- [ ] Accept or install the device HTTPS certificate only after confirming the
      expected device address and certificate fingerprint

### Runtime Security

- [ ] Monitor authentication logs
- [ ] Regularly update firmware
- [ ] Audit configuration changes
- [ ] Implement rate limiting for authentication attempts
- [ ] Use secure channels for remote access
- [ ] Reauthenticate when the HTTPS administrator session expires

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

The operational firmware configures 16 shared LWIP sockets. The HTTPS server
can use three internal sockets plus four client sockets; the remaining bounded
capacity is reserved for the read-only NUT service, DHCP/SNTP, and transient
browser or Agent connections. Reducing the global limit to ESP-IDF's default
of 10 can exhaust the shared socket table when a browser keeps HTTPS open and
can make both new HTTPS and NUT connections reset.

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
