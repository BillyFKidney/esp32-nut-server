# ESP32-NUT development preflight

Use this checklist at the start and end of any session that may interact with
the ESP32. The goals are to establish current facts quickly, keep a healthy
device undisturbed, and prevent multiple processes from competing for the COM
port.

Roles and authority are defined in
[ESP32_DEVELOPMENT_ROLES.md](ESP32_DEVELOPMENT_ROLES.md). Current addresses,
firmware, branch state, and the exact next action belong in
[ESP32_CURRENT_STATUS.md](ESP32_CURRENT_STATUS.md).

## Device Operator: start here

This is the normal pre-chat checklist. Stop after it unless the device is
unhealthy or the Maintainer/Agent asks for another action.

- [ ] Leave the ESP32 powered and leave the UPS connected to the ESP32 native
      USB host port when the system is healthy.
- [ ] Close any terminal tab or application actively running `idf.py monitor`,
      `screen`, PlatformIO Device Monitor, `minicom`, or another serial terminal.
- [ ] In UniFi, confirm ESP32-NUT is online and record its current IPv4 address
      and the observation time.
- [ ] Open `https://<current-ip>/` and record whether it responds. A browser
      certificate warning is expected during the self-signed-certificate
      milestone.
- [ ] If a NUT client is readily available, check
      `upsc cyberpower@<current-ip> ups.status`; expect `OL` while utility power
      is present. Otherwise report “NUT not checked.”
- [ ] Record whether the Mac mini COM cable is connected. Do not start a serial
      monitor.
- [ ] Start or resume the chat and provide this handoff:

```text
Observed at:
ESP32 IPv4 address:
HTTPS: responds / does not respond / not checked
NUT: status value / does not respond / not checked
COM cable: connected / disconnected
Serial monitor intentionally running: yes / no
Physical changes since last session:
```

It is not normally necessary to quit all of Visual Studio Code, Codex, or every
Code Helper process. Quit an owning application only when a later `lsof` check
proves that it still owns the COM device and it cannot release the port
normally.

Do not routinely disconnect and reconnect the UPS. That changes the USB state
being diagnosed and creates unnecessary power/backfeed risk. Review the
board-specific VBUS warning in [ESP32_README.md](ESP32_README.md) before changing
USB power connections.

## Project Maintainer preflight

Complete the Device Operator checklist first, then confirm:

- [ ] The requested session goal and acceptable stopping point are clear.
- [ ] Any physical reset, factory reset, flash, OTA, Git push, or other external
      change is explicitly authorized before it occurs.
- [ ] No credentials or private material will be pasted into chat, commands that
      echo output, serial logs, or tracked files.
- [ ] The current status document's repository and firmware claims still match
      the observed device or are clearly marked stale.
- [ ] The Agent is told about any intentional monitor, terminal, browser, or
      hardware state that must be preserved.

## Codex Agent preflight

Read these files before hardware work:

1. [ESP32_CURRENT_STATUS.md](ESP32_CURRENT_STATUS.md)
2. [ESP32_DEVELOPMENT_ROLES.md](ESP32_DEVELOPMENT_ROLES.md)
3. The active milestone requirements linked from the current status

Then inspect repository and COM state before opening a monitor or flashing:

```bash
git status --short --branch
ls -1 /dev/cu.usbmodem* 2>/dev/null
```

Choose the currently enumerated `/dev/cu.usbmodem...` path. Never assume its
suffix remains stable across reconnects. Check whether another process owns it,
replacing the example path with the current result:

```bash
lsof /dev/cu.usbmodem54E20396741
```

No `lsof` output means no process currently has that device open. If a process
is listed, record its PID and command. Stop that specific serial monitor
normally or request a human action; do not broadly terminate Code, Codex, or
unrelated helper processes.

Use the IP supplied by the Device Operator to check network services before
claiming the COM port:

```bash
nc -vz 192.168.40.173 443
nc -vz 192.168.40.173 3493
curl -k -sS -o /dev/null -w '%{http_code}\n' https://192.168.40.173/
upsc cyberpower@192.168.40.173 ups.status
```

Replace the example IP with the currently observed address. Record unavailable
tools or sandbox restrictions as “not checked”; do not reinterpret them as a
device failure. If the network services respond, serial is not required merely
to establish status.

If serial access is needed, keep one owner and one explicit device path for the
whole operation:

```bash
. /Users/billyfkidney/.espressif/v6.0.2/esp-idf/export.sh
idf.py -p /dev/cu.usbmodem54E20396741 monitor
```

Exit ESP-IDF Monitor with `Ctrl-]` and confirm with `lsof` that it released the
device before flashing, starting another monitor, or ending the session. Avoid
background monitors unless their PID and log destination are recorded.

## Connection recovery ladder

Stop as soon as a step restores access. Do not begin by unplugging everything.

1. **Refresh facts:** Recheck UniFi, the current IP, `/dev/cu.usbmodem*`, and
   `lsof` ownership. DHCP and USB device names may have changed.
2. **Network-only diagnosis:** Test TCP 443 and 3493. If either responds, the
   ESP32 is running; serial may be unnecessary.
3. **Release one serial owner:** Exit the identified monitor normally. If it is
   stuck, interrupt only that PID and verify the port is free with `lsof`.
4. **Reconnect only the COM cable:** If no `/dev/cu.usbmodem*` device appears,
   reconnect the Mac-to-COM cable and rediscover its path. Leave the UPS USB
   connection unchanged.
5. **Observe serial boot:** Open one monitor. Reset the ESP32 only when logs are
   required and a reset is safe for the current test.
6. **Reconnect the UPS USB cable:** Do this only when the ESP32 is healthy but
   native USB discovery or polling is absent. Observe disconnect/reconnect
   events in the single serial monitor.
7. **Flash over COM:** Use only when the installed image cannot be recovered by
   the authenticated OTA path or the test specifically requires a full flash.
   Record the exact port, commit, command, and result.
8. **Physical recovery:** Use the documented BOOT gestures only when their
   destructive scope is intended. A three-second hold clears Wi-Fi; a
   fifteen-second hold clears the agreed factory-reset data while retaining
   bootable firmware and the OTA recovery slot.

If a port remains busy after its owning application exits, record the `lsof`
output before escalating. That evidence is more useful than repeated cable
reconnections.

## Rules during a session

- Prefer OTA for normal application updates once the authenticated HTTPS route
  is verified; reserve COM flashing for recovery and tests that require it.
- Never run two serial monitors against the same device.
- Never assume an old IP address or `/dev/cu.usbmodem...` suffix is current.
- Record whether an installed image came from a clean commit or a dirty working
  tree. When dirty, record the base commit and later commit containing the
  tested change.
- Keep full serial captures untracked. Summarize decisive evidence in the
  current status and record the diagnostic filename and timestamp.
- Distinguish **observed**, **inferred**, and **not tested** status.
- Give physical instructions one action at a time and state what evidence to
  report before continuing.

## End-of-session checklist

### Codex Agent or Project Maintainer

- [ ] Record the active branch, HEAD, worktree state, and remote divergence.
- [ ] Record the firmware-reported version, active OTA slot if known, and
      whether rollback validation completed.
- [ ] Record the last verified IP, HTTPS result, NUT result, UPS identity/status,
      and meaningful serial errors.
- [ ] Record whether the COM port is connected, its last observed device path,
      and whether a monitor still owns it.
- [ ] Exit serial monitors cleanly and verify the port is released.
- [ ] State whether physical intervention is required before the next session.
- [ ] Write one exact next action and list anything awaiting authorization.
- [ ] Update the timestamp in `ESP32_CURRENT_STATUS.md`.

### Device Operator

- [ ] Complete any explicitly requested final GUI or physical observation.
- [ ] Confirm whether cables and power should remain as they are.
- [ ] Keep the ESP32 and UPS undisturbed when the system is healthy.
