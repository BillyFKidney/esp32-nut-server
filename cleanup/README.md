# Cleanup holding area

Nothing under this directory is an active build or documentation input. Files
and directories are preserved here for review instead of being deleted.

## Categories

| Path | Purpose |
| --- | --- |
| `artifacts/` | Large or machine-local diagnostic evidence retained outside normal Git status |
| `review/orphan/` | Material for which no active purpose or reference has been identified |
| `review/redundant/` | Duplicate material retained until a maintainer approves deletion |
| `delete_recommended/outdated/` | Material reviewed as obsolete but still preserved pending approval |

Moving something here is not permission to delete it. Record the source,
reason, date, and confidence before recommending deletion.

## 2026-07-15 cleanup inventory

### Empty duplicate directories

**Observed:** 39 directories ending in ` 2` existed alongside canonical
directories. Each duplicate contained zero files; the corresponding canonical
directory contained the tracked project content. They were moved without
deletion to:

`review/redundant/empty-directory-duplicates/`

Their original relative paths are preserved beneath that directory. The groups
were:

- `docs/`: `cables 2`, `images 2`, `man 2`
- `fatfs/`: `usr 2`, `var 2`
- `lib/`: `af_unix_socket 2`, `libusb 2`, `syslog 2`
- `scripts/`: 25 empty duplicate subdirectories
- `src/`: `common 2`, `drivers 2`, `server 2`
- `tests/`: `NIT 2`
- `tools/`: `nut-scanner 2`, `nutconf 2`

### Serial diagnostic capture

**Observed:** the untracked root file `screenlog.0` was a serial capture used to
diagnose and validate the Operational Management HTTPS startup fix. It was
moved without deletion to:

`artifacts/serial/2026-07-15-https-foundation-screenlog.0`

The decisive evidence is summarized in `docs/ESP32_CURRENT_STATUS.md`.

## Structure reviewed but retained

The numerous files at repository root and directly under `docs/` are primarily
tracked upstream NUT build inputs, licenses, CI definitions, documentation
sources, and generated-document configuration. They are not orphans and remain
in their canonical locations.

Expected generated or machine-local directories and files such as `build/`,
`managed_components/`, `dependencies.lock`, and `sdkconfig` remain where
ESP-IDF expects them and are ignored by Git.

## Proposed organization requiring review

Consider creating `docs/esp32/` in a separate, reviewable change and moving the
ESP32-specific Markdown documents there. A possible layout is:

```text
docs/esp32/
├── README.md
├── SECURITY.md
├── DEVELOPMENT_PLAN.md
├── CURRENT_STATUS.md
├── DEVELOPMENT_ROLES.md
├── PREFLIGHT.md
├── milestones/
│   └── OPERATIONAL_MANAGEMENT_QA.md
└── testing/
    └── MANUAL_TEST_PLAN.md
```

Benefits would be a shorter `docs/` listing and a clear downstream boundary.
Costs are link updates, possible documentation-build changes, and more path
divergence from existing commits. Do not perform this move until references and
the desired naming scheme are reviewed by the Project Maintainer.
