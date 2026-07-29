# ESP32-NUT agent instructions

These repository-wide instructions are a routing page and safety boundary. They
are intentionally short so a new agent can begin useful work without loading
the project history.

## Fast start

1. Read [docs/ESP32_CURRENT_STATUS.md](docs/ESP32_CURRENT_STATUS.md).
2. Run `git status --short --branch`, resolve the live HEAD, and confirm that
   the file's branch, base, and worktree claims are still current.
3. Read only the task-specific document selected from the routing table below.

Stop there unless the task requires another reference. Do **not** preload the
development plan, milestone Q&A, roles, preflight, or `docs/archive/` during an
ordinary startup.

## Task routing

| Task or question | Read next |
| --- | --- |
| Current branch, active scope, next action | [ESP32_CURRENT_STATUS.md](docs/ESP32_CURRENT_STATUS.md) |
| Continuing with a 64k-context agent | [ESP32_64K_AGENT_HANDOFF.md](docs/ESP32_64K_AGENT_HANDOFF.md) |
| Roadmap, version, or branch boundary | [ESP32_DEVELOPMENT_PLAN.md](docs/ESP32_DEVELOPMENT_PLAN.md) |
| Locked Operational Management requirement or decision | [ESP32_DEVELOPMENT_MILESTONE_QA_OPERATIONAL_MANAGEMENT.md](docs/ESP32_DEVELOPMENT_MILESTONE_QA_OPERATIONAL_MANAGEMENT.md) |
| Hardware, LAN, COM, build, flash, OTA, or recovery | [ESP32_PREFLIGHT.md](docs/ESP32_PREFLIGHT.md) |
| Authority for physical, destructive, GitHub, or external actions | [ESP32_DEVELOPMENT_ROLES.md](docs/ESP32_DEVELOPMENT_ROLES.md) |
| Security or authorization behavior | [ESP32_SECURITY.md](docs/ESP32_SECURITY.md) |
| Management/Wi-Fi modular refactoring | [ESP32_REFACTORING_PLAN.md](docs/ESP32_REFACTORING_PLAN.md) |
| Synology/AdGuard browser access | [ESP32_MANAGEMENT_PROXY.md](docs/ESP32_MANAGEMENT_PROXY.md) |
| Moving tracked files or changing repository layout | [ESP32_REPOSITORY_LAYOUT.md](docs/ESP32_REPOSITORY_LAYOUT.md) |
| Detailed downstream port/build notes | [ESP32_README.md](docs/ESP32_README.md) |
| Historical releases, validation, or superseded guidance | [docs/archive/README.md](docs/archive/README.md) |

The root [README.md](README.md) is the application landing page. The repository
is a downstream ESP32-S3 port built on the upstream Network UPS Tools source
and architecture; inherited source, build, legal, and compatibility files are
not disposable clutter.

## Non-negotiable guardrails

- Treat existing modifications and untracked files as user-owned until their
  provenance is established. Never discard work with destructive Git commands.
- Do not delete during cleanup unless the Project Maintainer explicitly
  authorizes deletion. Preserve inactive material according to
  [cleanup/README.md](cleanup/README.md).
- Before moving tracked files, follow the repository-layout policy and obtain
  Project Maintainer approval. Update every build and documentation reference.
- Keep generated ESP-IDF state untracked: `build/`, `managed_components/`,
  `dependencies.lock`, local `sdkconfig`, and machine/editor settings.
- Target YD-ESP32-23 / ESP32-S3-WROOM-1-N16R8 with ESP-IDF v6.0.2.
- Keep the existing NUT daemon and driver architecture. UPS access remains
  read-only until a separately reviewed safety model authorizes controls.
- Preserve LAN-only HTTPS `443`, read-only NUT `3493`, and refusal of the
  retired unauthenticated `8080` service.
- Do not perform expensive certificate or HTTPS startup work in the ESP-IDF
  system event task; commit `0fcd9e1f9` records the prior `sys_evt` overflow.
- Never record passwords, Wi-Fi credentials, cookies, API tokens, private keys,
  or Authorization headers in source, tracked documentation, logs, or chat.
- Do not retire a service, remove Agent/operator capability, or transfer a
  recurring workflow to a human without explicit Project Maintainer approval
  covering impact, replacement, validation, and rollback.
- Flashing, OTA, physical reset/recovery, pushing, merging, tagging, releasing,
  or other external/destructive actions require authority in the roles file and
  the current user request. Documentation or implementation work alone does not
  imply that authority.

## Working method

- Keep each branch to one coherent, reviewable acceptance boundary.
- Prefer network evidence and rediscover current IP/USB coordinates; historical
  addresses and `/dev/cu.usbmodem*` suffixes are evidence, not current facts.
- Use one serial-monitor owner. Follow the preflight before touching hardware.
- Build with the installed ESP-IDF v6.0.2 environment and validate changes on
  the ESP32-S3 target in proportion to risk before calling them complete.
- Distinguish **observed**, **inferred**, and **not tested**. Do not turn missing
  access or an unavailable tool into a device-failure claim.
- Preserve behavior outside the active slice, especially ADMIN/CSRF boundaries,
  service ports, read-only UPS access, and rollback/recovery paths.
- Keep `management.c` and `wifi.c` as orchestration boundaries while extracting
  one focused concern per branch; do not mix refactoring with behavior changes.
- Register each new focused `src/*.c` module explicitly in
  `src/CMakeLists.txt`; the legacy recursive source glob does not notice new
  files during an incremental ESP-IDF build.

## Handoff discipline

Keep [ESP32_CURRENT_STATUS.md](docs/ESP32_CURRENT_STATUS.md) lightweight. Update
it only with current branch/base/worktree facts, implementation and validation
state, authorization or blockers, and one exact next action. Put reusable
procedures in preflight, roadmap scope in the development plan, decisions in
the milestone/security documents, and completed evidence in `docs/archive/`.
Archives are reference material and are not part of normal agent startup.

At session end, report the active branch, HEAD, worktree and remote divergence,
what changed, validation performed, what was not tested, and the exact next
action. Do not push, merge, tag, OTA, flash, or release unless explicitly
requested.
