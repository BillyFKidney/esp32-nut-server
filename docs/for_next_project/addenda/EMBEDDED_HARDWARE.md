# Embedded and connected-hardware addendum

Apply these sections to `AGENTS.md`, `PROJECT_BRIEF.md`, `PREFLIGHT.md`, current
status, security, and testing when software interacts with physical hardware.

## Record exact target identity

- Board/product and revision: `{{BOARD_AND_REVISION}}`
- Module/chip: `{{MODULE_AND_CHIP}}`
- Flash/RAM/storage: `{{MEMORY}}`
- SDK/toolchain and exact version: `{{SDK_VERSION}}`
- Power sources and electrical constraints: `{{POWER_BOUNDARY}}`
- Data/debug/host ports: `{{PORT_PURPOSES}}`
- Connected peripherals and identifiers: `{{PERIPHERALS}}`

Do not generalize validation from a development board to another revision or
from one peripheral model to an entire device class without evidence.

## Hardware guardrails

- Document voltage, polarity, backfeed, grounding, current, bus ownership, and
  boot-mode hazards before asking a human to reconnect hardware.
- Keep one owner for exclusive serial, USB, debugger, radio, or programmer
  resources.
- Discover device paths and addresses each session; do not hard-code suffixes
  that can change after reconnect.
- Give one physical instruction at a time, state the expected observation, and
  wait for evidence before the next action.
- Prefer network/status checks before resets, cable changes, erasure, or flash.

## Firmware provenance

Current status should record:

| Field | Value |
| --- | --- |
| Source branch/commit | {{BRANCH_AND_COMMIT}} |
| Dirty or clean build | {{STATE_AND_DIFF_PROVENANCE}} |
| Build/toolchain version | {{VERSIONS}} |
| Artifact identifier/hash | {{IDENTIFIER}} |
| Installed slot/bank/version | {{SLOT_OR_VERSION}} |
| Rollback state | {{PENDING_VALID_COMPLETE_NOT_SUPPORTED}} |
| Installation method | {{FLASH_OTA_PROGRAMMER_OTHER}} |

A dirty build may be valid evidence only when its diff is preserved and later
committed. Record that relationship explicitly.

## Hardware preflight additions

- [ ] Preserve healthy power and peripheral connections.
- [ ] Record visible LEDs/displays and externally observable behavior.
- [ ] Discover the current serial/debug/USB path and its owning process.
- [ ] Confirm network address and primary services without claiming serial.
- [ ] Record which cable carries power versus data.
- [ ] Confirm backup/recovery image, slot, or reflashing path.
- [ ] Ensure no automatic monitor or IDE task will compete for the port.

## Recovery ladder additions

1. Refresh network and USB/serial enumeration.
2. Release the specific resource owner.
3. Observe without reset when possible.
4. Reset only the processor or affected peripheral.
5. Reconnect only the affected data cable.
6. Restore known-good firmware via the least destructive supported path.
7. Use bootloader/programmer/factory recovery only with explicit scope and
   preserved evidence.

## Validation matrix

| Layer | Evidence |
| --- | --- |
| Build | Clean supported toolchain; size/resource checks |
| Boot | Reset reason, version, configuration, watchdog/rollback state |
| Peripheral | Enumeration, identity, read/write safety, reconnect behavior |
| Network/protocol | Reachability and real client interoperability |
| Runtime | Sustained operation, memory/stack/resource behavior |
| Upgrade | Install, restart, validation, rollback, corrupt-image rejection |
| Recovery | Documented physical and remote recovery paths |
| User acceptance | Required phone/computer/browser/control surface |

Do not call hardware work complete based only on compilation, simulation, or a
single successful boot.
