# ESP32-NUT repository layout

This document explains which files belong at the repository root and which
supporting material has a dedicated directory. The layout is intentionally
conservative because ESP32-NUT is still built from a downstream NUT source
tree.

## Root policy

The root is reserved for project entry points, build inputs, legal metadata,
repository-wide tooling, and compatibility files whose consumers expect the
root path.

| Category | Root policy | Examples |
| --- | --- | --- |
| Application entry points | Keep | `README.md`, `README.adoc`, `CMakeLists.txt`, `platformio.ini` |
| ESP-IDF configuration | Keep | `sdkconfig.defaults`, `version.txt` |
| Legal and attribution | Keep | `COPYING`, `LICENSE-*`, `AUTHORS`, `MAINTAINERS` |
| Repository-wide controls | Keep | `.editorconfig`, `.gitattributes`, `.gitignore`, CodeQL marker |
| NUT Autotools compatibility | Keep while supported | `configure.ac`, `Makefile.am`, `autogen.sh`, `compile` |
| Active CI entry points | Keep while consumed externally | `ci_build.sh`, `.circleci/`, GitHub workflows |

Root files are not required to be part of the ESP32 runtime to belong there.
Build, packaging, legal, and repository tools are project inputs and should not
be moved merely because the firmware does not open them at runtime.

## Application and support directories

* `src/`, `include/`, `lib/`, `conf/`, `fatfs/`, and `tests/` contain the
  application and its inherited NUT implementation.
* `boards/partitions/` contains board-specific ESP-IDF and PlatformIO
  partition tables. The current `nut_fat_8MB.csv` path is referenced by both
  `platformio.ini` and `sdkconfig.defaults`.
* `docs/` contains application procedures, handoffs, security decisions, and
  detailed downstream notes.
* `docs/upstream/` contains the retained upstream NUT narrative documents:
  `INSTALL.nut.adoc`, `NEWS.adoc`, `TODO.adoc`, and `UPGRADING.adoc`.
* `scripts/` and `tools/` contain reusable build, driver, packaging, and
  maintenance helpers. The source-formatting helper is now `tools/indent.sh`.
* `.github/` contains the active GitHub repository configuration and workflows.

Historical rationale for the 2026 upstream-document and CI-file preservation
decisions is retained in
[ESP32_REPOSITORY_LAYOUT_HISTORY.md](archive/ESP32_REPOSITORY_LAYOUT_HISTORY.md).

## Generated and machine-local files

The following are local build outputs or machine state and are not part of the
published repository layout:

* `build/`
* `managed_components/`
* `dependencies.lock`
* local `sdkconfig` files
* `.DS_Store`

They remain ignored and should not be moved into source directories.

## Layout change validation

Any future layout change must verify all of the following before merge:

1. ESP-IDF and PlatformIO can still locate the board configuration and
   partition table.
2. The inherited NUT documentation generation and packaging references resolve
   after any upstream-file move.
3. Active GitHub and external CI entry points still invoke the intended files.
4. `README.md`, `README.adoc`, `AGENTS.md`, and current status route agents to
   the correct application documents.
5. The change does not alter firmware behavior, partition contents, service
   ports, authorization boundaries, or release artifacts unless that is its
   explicitly reviewed scope.
