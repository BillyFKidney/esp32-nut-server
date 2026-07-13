# esp32-nut-server

**Status:** IN PROGRESS — upstream adoption and repository strategy

This Git repository is intentionally reduced to its history, license, ignore rules, and architecture direction. The earlier standalone USB enumeration proof of concept was removed before further development so the project does not recreate work already present in Network UPS Tools.

For a standalone continuation brief, use [`new-chat-prompt.md`](new-chat-prompt.md).

## Required starting point

The next development plan must begin with a detailed review and adoption strategy for:

- NUT fork: [`banoz/nut`](https://github.com/banoz/nut/tree/esp32-alpha)
- Branch: `esp32-alpha`
- ESP32 guide: [`docs/ESP32_README.md`](https://github.com/banoz/nut/blob/esp32-alpha/docs/ESP32_README.md)

Treat that branch as the candidate foundation, not merely as inspiration. Determine whether to rebase, merge, fork, or selectively port only after comparing its build system, supported drivers, hardware assumptions, networking, storage, licensing, maintenance state, and compatibility with ESP-IDF 6.0.2 and ESP32-S3 USB host operation.

## Known target hardware

- Board: YD-ESP32-23
- Module: ESP32-S3-WROOM-1-N16R8
- Flash: 16 MB
- PSRAM: 8 MB octal at 80 MHz
- Development target: `esp32s3`
- Last verified ESP-IDF: v6.0.2
- Programming/monitor USB port: `/dev/cu.usbmodem1101` on the original development Mac
- Second USB-C port: USB host connection to a USB HID UPS

Machine-specific port names are observations, not portable configuration defaults.

## Before code resumes

1. Confirm the parent workspace validation report approves resumption.
2. Inventory the `esp32-alpha` branch and its history, licenses, dependencies, build instructions, and open limitations.
3. Write an adoption plan that preserves upstream structure and minimizes divergence.
4. Reconcile hardware findings with the reusable module guide and the archived proof-of-concept note.
5. Define a minimal build-and-boot milestone before adding UPS or NUT behavior.

Do not restore the discarded proof-of-concept implementation as the default architecture. Its tracked history remains available in Git, and its hardware note is archived in the parent workspace at `archive/esp32-nut-server-2026-07-13/`.
