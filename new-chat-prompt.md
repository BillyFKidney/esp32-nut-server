# New Chat Prompt: ESP32-NUT

Copy everything below into a new chat.

---

Continue development of ESP32-NUT using the existing `banoz/nut` ESP32 port as the foundation. Do not recreate Network UPS Tools or continue the discarded standalone USB-enumeration architecture.

## Workspace and repository

- Workspace: `/Users/billyfkidney/Documents/squirrel-powered-labs`
- Local Git repository: `/Users/billyfkidney/Documents/squirrel-powered-labs/embedded/esp32-nut-server`
- Current local branch: `feature/usb-enumeration`
- Required candidate foundation: `https://github.com/banoz/nut/tree/esp32-alpha`
- Required ESP32 guide: `https://github.com/banoz/nut/blob/esp32-alpha/docs/ESP32_README.md`
- Workspace status: `IN PROGRESS`

The local repository was intentionally reset. Its Git history, `.gitignore`, `LICENSE`, and direction README remain; the previous proof-of-concept source and generated ESP-IDF state were removed. Expect intentional tracked deletions on the current branch. Do not restore that implementation merely to make the worktree clean, and do not rewrite Git history without explicit approval.

## Hardware and toolchain

- ESP-IDF: v6.0.2
- Target: `esp32s3`
- Board: YD-ESP32-23
- Module: ESP32-S3-WROOM-1-N16R8
- Flash: 16 MB
- PSRAM: 8 MB octal at 80 MHz
- Known programming/monitor port on the original Mac: `/dev/cu.usbmodem1101`
- The other USB-C port connects to a USB HID UPS and is intended for USB host mode.

Reusable hardware documentation is under:

`/Users/billyfkidney/Documents/squirrel-powered-labs/documentation/hardware/ESP32/S3/ESP32-S3-WROOM-1-N16R8/`

The discarded proof-of-concept hardware note is retained only as historical evidence under:

`/Users/billyfkidney/Documents/squirrel-powered-labs/archive/esp32-nut-server-2026-07-13/`

## Required first phase: inspect before editing

1. Read the workspace root `README.md`, `STATUS.md`, and current validation report.
2. Inspect the complete local repository, Git status, tracked history, branches, tags, and remotes.
3. Review the complete `banoz/nut` `esp32-alpha` branch, its `docs/ESP32_README.md`, build system, ESP32-specific source, history, dependencies, licenses, documented limitations, and relationship to upstream NUT.
4. Determine whether the local repository should adopt that work through a fork/remote strategy, branch replacement, merge, rebase, or another minimally divergent method. Explain the consequences for history, future upstream synchronization, and the existing GitHub repository.
5. Compare the ESP32 port’s documented target/toolchain assumptions with ESP-IDF v6.0.2, ESP32-S3, native USB host requirements, flash, and PSRAM configuration.
6. Consult the installed ESP-IDF v6.0.2 USB host examples when relevant:
   - `$IDF_PATH/examples/peripherals/usb/host/usb_host_lib`
   - `$IDF_PATH/examples/peripherals/usb/host/hid`

Before writing code or performing destructive Git operations, provide:

- a candid technical assessment of the `esp32-alpha` port;
- what can be reused unchanged;
- what is incompatible, stale, risky, or missing;
- the recommended repository/upstream adoption strategy;
- the exact proposed file and Git changes;
- a small sequence of measurable milestones;
- only the questions whose answers would materially change that plan.

## Implementation direction

After the adoption plan is reviewed, begin with the smallest foundation milestone that proves the chosen upstream tree builds and boots on this exact ESP32-S3 board. Preserve upstream structure and minimize divergence.

The later USB milestone is read-only UPS discovery:

1. Initialize the ESP-IDF USB host library.
2. Detect connect and disconnect events.
3. Open the UPS device.
4. Print VID and PID.
5. Print manufacturer, product, and serial strings when available.
6. Print configuration and interface descriptors.
7. Handle disconnect cleanly.

Do not build a separate NUT protocol implementation before understanding what `banoz/nut` already supplies. Keep generated build files, managed components, dependency locks when project policy treats them as generated, local `sdkconfig`, and machine-specific editor settings untracked.

Follow repository and upstream conventions. For new project-owned C code when no upstream convention governs it: files and directories use kebab-case, variables and functions use snake_case, types use PascalCase, and constants use UPPER_SNAKE_CASE.

Validate every material change proportionally. Do not flash hardware until the build succeeds and the proposed operation is appropriate for the selected branch state.

---
