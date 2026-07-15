# ESP32-NUT agent instructions

These instructions apply to the whole repository.

## Repository identity

This is a downstream ESP32-S3 port built on the upstream Network UPS Tools
(NUT) repository. The large root, `docs/`, `scripts/`, `src/`, `tests/`, and
`tools/` layouts are intentional upstream structure, not miscellaneous project
clutter. Preserve that structure so builds, documentation generation, Git
history, and comparisons with `banoz/nut` and `networkupstools/nut` remain
valid.

Do not relocate tracked upstream files merely to reduce the number of entries
shown in an editor. Before moving any tracked file, identify all build and
documentation references and obtain Project Maintainer approval.

## Start here

Before hardware or implementation work, read:

1. `docs/ESP32_CURRENT_STATUS.md`
2. `docs/ESP32_DEVELOPMENT_ROLES.md`
3. `docs/ESP32_PREFLIGHT.md`
4. The active milestone document linked from current status

Use the role names **Device Operator**, **Project Maintainer**, and **Codex
Agent**. Uppercase `ADMIN` and `USER` refer only to ESP32-NUT product accounts.

## Working-tree safety

- Treat all existing modifications and untracked files as user-owned until
  their provenance is established.
- Use `git status --short --branch`, `git ls-files`, and `git check-ignore`
  before classifying a file as orphaned, redundant, or generated.
- Never delete files during cleanup work unless the Project Maintainer
  explicitly authorizes deletion.
- Do not use destructive Git commands to discard work.
- Keep generated ESP-IDF outputs untracked: `build/`, `managed_components/`,
  `dependencies.lock`, and local `sdkconfig` files.
- Keep large serial captures and other reproducible diagnostics under
  `cleanup/artifacts/`; summarize decisive evidence in current status.

## Cleanup policy

Use `cleanup/` only for inactive material:

- `cleanup/artifacts/`: diagnostic captures retained outside normal Git status
- `cleanup/review/orphan/`: material with no identified active purpose
- `cleanup/review/redundant/`: duplicate material retained for review
- `cleanup/delete_recommended/outdated/`: material reviewed as obsolete but not
  deleted

Preserve a moved item's relative path when practical. Update
`cleanup/README.md` with the reason, source, destination, date, and whether the
classification was observed or inferred. Do not move active source,
configuration, build inputs, or documentation into `cleanup/`.

## ESP32 implementation guardrails

- Target YD-ESP32-23 / ESP32-S3-WROOM-1-N16R8 with ESP-IDF v6.0.2.
- Keep the existing NUT daemon and driver architecture.
- UPS access remains read-only until a separate safety review authorizes
  control operations.
- Do not restore the unauthenticated development OTA listener on TCP port 8080.
- Operational management is LAN-only HTTPS on TCP port 443; NUT remains
  read-only on TCP port 3493.
- Do not perform expensive certificate or HTTPS startup work in the ESP-IDF
  system event task. Commit `0fcd9e1f9` moved that work after a `sys_evt` stack
  overflow.
- Validate changes on the target ESP32-S3 in proportion to their risk before
  calling them complete.

## Build and hardware workflow

Use the installed ESP-IDF environment:

```bash
. /Users/billyfkidney/.espressif/v6.0.2/esp-idf/export.sh
idf.py build
```

Discover `/dev/cu.usbmodem*` at the start of each session. Never hard-code a
previous USB suffix. Use network checks before claiming the serial port, keep
only one serial-monitor owner, exit ESP-IDF Monitor with `Ctrl-]`, and verify
port release with `lsof`.

Flashing, OTA installation, physical recovery, pushing, releasing, or other
external/destructive actions require the authority described in
`docs/ESP32_DEVELOPMENT_ROLES.md` and the current user request.

## Documentation and handoff

- Keep `docs/ESP32_CURRENT_STATUS.md` factual and current when repository,
  firmware, hardware, validation, or next-action state changes.
- Keep reusable procedures in `docs/ESP32_PREFLIGHT.md`, not in current status.
- Mark claims as **observed**, **inferred**, or **not tested** where ambiguity
  matters.
- Record one exact next action at session end.
- Never record passwords, Wi-Fi credentials, session cookies, API tokens,
  private keys, or other secrets in tracked files or chat.
